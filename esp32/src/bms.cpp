#include "bms.h"

#include <Arduino.h>
#include <driver/twai.h>
#include <math.h>

#include "vcu.h"

namespace {

// Waveshare ESP32-S3-Touch-LCD-4.3: bộ thu phát CAN nối GPIO15 (TX) / GPIO16 (RX)
constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_15;
constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_16;

constexpr float VOLTAGE_SCALE_V_PER_COUNT = 0.002f;
constexpr float CURRENT_SCALE_A_PER_COUNT = 0.02f;
constexpr uint32_t LIVE_TIMEOUT_MS = 1200;     // giống CAN_LIVE_TIMEOUT_MS bên code_sac
constexpr uint32_t FIELD_TIMEOUT_MS = 5000;    // giữ giá trị cũ tối đa 5 s
constexpr uint32_t RETRY_MS = 2000;

BmsData raw;            // giá trị mới nhất (chưa xét hết hạn)
BmsData view;           // bản trả ra ngoài
uint32_t lastFrameMs = 0, lastPackMs = 0, lastSocMs = 0, lastTempMs = 0;
uint8_t cell20Streak = 0;

bool driverReady = false;
bool recovering = false;
uint32_t lastInitMs = 0, lastStatusMs = 0, lastLogMs = 0;
uint32_t framesTotal = 0, framesLogged = 0;

uint16_t u16be(const uint8_t *d) { return (uint16_t(d[0]) << 8) | d[1]; }
int16_t i16be(const uint8_t *d) { return (int16_t)u16be(d); }

bool fresh(uint32_t t, uint32_t now, uint32_t timeout) { return t != 0 && now - t <= timeout; }

bool startCan() {
    lastInitMs = millis();
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    g.tx_queue_len = 0;      // không bao giờ gửi
    g.rx_queue_len = 64;
    twai_timing_config_t timing = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&g, &timing, &filter);
    if (err != ESP_OK) {
        Serial.printf("[CAN] cài driver lỗi: %d\n", (int)err);
        return driverReady = false;
    }
    if ((err = twai_start()) != ESP_OK) {
        Serial.printf("[CAN] start lỗi: %d\n", (int)err);
        twai_driver_uninstall();
        return driverReady = false;
    }
    recovering = false;
    Serial.println("[CAN] TWAI 250 kbps, chỉ nghe BMS");
    return driverReady = true;
}

void reinstall(uint32_t now) {
    twai_stop();
    twai_driver_uninstall();
    driverReady = false;
    recovering = false;
    lastInitMs = now;
}

void serviceBus(uint32_t now) {
    if (!driverReady) {
        if (now - lastInitMs >= RETRY_MS) startCan();
        return;
    }
    if (now - lastStatusMs < 50) return;
    lastStatusMs = now;

    twai_status_info_t st = {};
    if (twai_get_status_info(&st) != ESP_OK) {
        reinstall(now);
        return;
    }
    if (st.state == TWAI_STATE_BUS_OFF && !recovering) {
        Serial.println("[CAN] BUS-OFF, đang khôi phục");
        recovering = twai_initiate_recovery() == ESP_OK;
    } else if (st.state == TWAI_STATE_STOPPED) {
        if (twai_start() == ESP_OK) recovering = false;
    } else if (st.state == TWAI_STATE_RUNNING) {
        recovering = false;
    }

    if (now - lastLogMs >= 2000) {
        lastLogMs = now;
        Serial.printf("[CAN] state=%d rx/2s=%lu V=%.2f I=%.2f SOC=%.0f T=%.0f/%.0f/%.0f cells=%u %s\n",
                      (int)st.state, (unsigned long)(framesTotal - framesLogged),
                      raw.voltage, raw.current, raw.soc, raw.temps[0], raw.temps[1], raw.temps[2],
                      raw.cellCount, bmsOnline() ? "ONLINE" : "OFFLINE");
        framesLogged = framesTotal;
    }
}

void decode(const twai_message_t &f, uint32_t now) {
    if (f.extd || f.rtr) return;
    const uint8_t *d = f.data;
    const uint8_t n = f.data_length_code;

    vcuDecode(f.identifier, d, n, now);   // khung của xe (0x102, 0x201, 0x400)

    switch (f.identifier) {
        case 0x309: {
            if (n < 8) return;
            const float v = u16be(&d[2]) * VOLTAGE_SCALE_V_PER_COUNT;
            const float a = i16be(&d[6]) * CURRENT_SCALE_A_PER_COUNT;
            if (v < 40 || v > 80 || a < -300 || a > 300) return;
            raw.voltage = v;
            raw.current = a;
            lastPackMs = lastFrameMs = now;
            break;
        }
        case 0x30A: {
            if (n < 6) return;
            const uint8_t idx = raw.cellCount == 20 ? 2 : 1;
            if (d[idx] <= 100) {
                raw.soc = d[idx];
                lastSocMs = now;
            }
            raw.sohValid = d[3] <= 100;
            if (raw.sohValid) raw.soh = d[3];
            lastFrameMs = now;
            break;
        }
        case 0x31A: {
            if (n < 6) return;
            const float c20 = n >= 8 ? u16be(&d[6]) / 10000.0f : NAN;
            if (isfinite(c20) && c20 >= 2.0f && c20 <= 4.0f) {
                if (cell20Streak < 3) ++cell20Streak;
                if (cell20Streak >= 3) raw.cellCount = 20;
            } else {
                cell20Streak = 0;
                raw.cellCount = 19;
            }
            lastFrameMs = now;
            break;
        }
        case 0x311: case 0x312: case 0x313: case 0x314:
            if (n >= 8) lastFrameMs = now;
            break;
        case 0x320:
            if (n < 4) return;
            raw.temps[0] = d[0];
            raw.temps[1] = d[1];
            raw.temps[2] = d[3];
            lastTempMs = lastFrameMs = now;
            break;
    }
}

}  // namespace

float BmsData::maxTemp() const {
    return fmaxf(temps[0], fmaxf(temps[1], temps[2]));
}

void bmsBegin() {
    startCan();
}

void bmsPoll() {
    const uint32_t now = millis();
    serviceBus(now);

    if (driverReady && !recovering) {
        twai_message_t f;
        for (int i = 0; i < 32 && twai_receive(&f, 0) == ESP_OK; i++) {
            ++framesTotal;
            decode(f, millis());
        }
    }

    view = raw;
    view.packValid = fresh(lastPackMs, now, FIELD_TIMEOUT_MS);
    view.socValid = fresh(lastSocMs, now, FIELD_TIMEOUT_MS);
    view.tempValid = fresh(lastTempMs, now, FIELD_TIMEOUT_MS);
    view.sohValid = raw.sohValid && view.socValid;
}

bool bmsOnline() {
    return fresh(lastFrameMs, millis(), LIVE_TIMEOUT_MS);
}

const BmsData &bmsData() {
    return view;
}
