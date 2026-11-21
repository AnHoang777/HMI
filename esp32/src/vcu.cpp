#include "vcu.h"

#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include <stdio.h>

namespace {

// ---------------------------------------------------------------- bảng chế độ 0x102 D0
// Điền khi đã dò được giá trị, ví dụ: {0x00, "P", 0xFFB020, true}, {0x02, "ECO", 0x3DF26A, false}
struct ModeEntry {
    uint8_t raw;
    const char *name;
    uint32_t color;
    bool park;
};
constexpr ModeEntry MODE_TABLE[] = {
    {0xFF, nullptr, 0, false},   // chỗ giữ chỗ, chưa có giá trị thật
};

constexpr uint32_t FRESH_MS = 3000;
constexpr uint32_t MAX_GAP_MS = 500;          // khoảng trống lớn hơn thì không tích phân tốc độ
constexpr float MIN_CALIB_KM = 0.5f;          // cần ít nhất 0,5 km mới tính hệ số
constexpr float SAVE_EVERY_KM = 1.0f;

Preferences prefs;
VcuData v;

uint32_t last102Ms = 0, last201Ms = 0;
bool haveOdo = false;
uint32_t tripStartRaw = 0;
bool tripStartSet = false;

// hiệu chỉnh tốc độ: ∫raw·dt giữa các lần ODO tăng
uint32_t lastSpeedMs = 0;
uint16_t prevSpeedRaw = 0;
double rawHours = 0;          // tích phân raw theo giờ kể từ mốc ODO
bool calRunning = false;
uint16_t calStartOdo = 0;
float savedCalibKm = 0;       // quãng đường của hệ số đang lưu trong NVS
float lastSavedSessionKm = 0;

uint32_t lastLogMs = 0;

uint16_t u16be(const uint8_t *d) { return (uint16_t(d[0]) << 8) | d[1]; }
bool fresh(uint32_t t, uint32_t now) { return t != 0 && now - t <= FRESH_MS; }

void saveScale(float scale, float km) {
    prefs.putFloat("spdScale", scale);
    prefs.putFloat("spdKm", km);
    savedCalibKm = km;
}

void onOdo(uint16_t odo) {
    if (!haveOdo) {
        haveOdo = true;
        v.odoRaw = odo;
        if (!tripStartSet || odo < tripStartRaw) {
            tripStartRaw = odo;
            tripStartSet = true;
            prefs.putUInt("trip0", tripStartRaw);
        }
        return;
    }
    if (odo == v.odoRaw) return;

    const bool normalStep = odo > v.odoRaw && odo - v.odoRaw <= 5;
    v.odoRaw = odo;
    if (odo < tripStartRaw) {
        tripStartRaw = odo;
        prefs.putUInt("trip0", tripStartRaw);
    }

    if (!normalStep) {          // ODO nhảy bất thường → bắt đầu đo lại
        calRunning = false;
        return;
    }
    if (!calRunning) {          // mốc đầu tiên: bắt đầu tích phân đúng lúc ODO vừa tăng
        calRunning = true;
        calStartOdo = odo;
        rawHours = 0;
        lastSavedSessionKm = 0;
        return;
    }

    const float km = (odo - calStartOdo) / 10.0f;
    if (km < MIN_CALIB_KM || rawHours <= 0) return;
    const float scale = (float)(km / rawHours);
    // Hệ số đã lưu (đo trên quãng dài) chỉ bị thay khi chuyến này đi đủ dài (tối đa yêu cầu 2 km)
    if (v.calibrated && km < fminf(savedCalibKm, 2.0f)) return;
    v.speedScale = scale;
    v.calibKm = km;
    v.calibrated = true;
    if (km - lastSavedSessionKm >= SAVE_EVERY_KM) {
        lastSavedSessionKm = km;
        saveScale(scale, km);
    }
}

}  // namespace

const char *vcuModeName(uint8_t raw, uint32_t *color, bool *isPark) {
    for (const ModeEntry &m : MODE_TABLE) {
        if (m.name && m.raw == raw) {
            if (color) *color = m.color;
            if (isPark) *isPark = m.park;
            return m.name;
        }
    }
    return nullptr;
}

void vcuBegin() {
    prefs.begin("vcu", false);
    v.speedScale = prefs.getFloat("spdScale", 0);
    savedCalibKm = prefs.getFloat("spdKm", 0);
    v.calibrated = v.speedScale > 0;
    v.calibKm = v.calibrated ? savedCalibKm : 0;
    tripStartSet = prefs.isKey("trip0");
    tripStartRaw = prefs.getUInt("trip0", 0);
}

void vcuDecode(uint32_t id, const uint8_t *d, uint8_t len, uint32_t now) {
    switch (id) {
        case 0x102:
            if (len < 8) return;
            v.stateRaw = d[0];
            v.standBit = d[1] & 0x10;
            v.dist100m = d[7];
            last102Ms = now;
            break;
        case 0x201: {
            if (len < 8) return;
            const uint16_t raw = u16be(&d[0]);
            if (lastSpeedMs && now - lastSpeedMs <= MAX_GAP_MS && calRunning) {
                // hình thang giữa hai mẫu
                rawHours += (prevSpeedRaw + raw) * 0.5 * (now - lastSpeedMs) / 3600000.0;
            }
            lastSpeedMs = now;
            prevSpeedRaw = raw;
            v.speedRaw = raw;
            v.brakeBit = d[7] & 0x10;
            last201Ms = now;
            onOdo(u16be(&d[4]));
            break;
        }
        case 0x400:
            if (len < 8) return;
            v.odo400 = d[7];
            break;
    }
}

void vcuUpdate() {
    const uint32_t now = millis();
    v.stateValid = fresh(last102Ms, now);
    v.motionValid = fresh(last201Ms, now);

    v.speedKmh = (v.motionValid && v.calibrated) ? v.speedRaw * v.speedScale : NAN;
    v.odoKm = (v.motionValid && haveOdo) ? v.odoRaw / 10.0f : NAN;
    v.tripKm = (v.motionValid && haveOdo && tripStartSet) ? (v.odoRaw - tripStartRaw) / 10.0f
                                                          : NAN;

    if (now - lastLogMs >= 1000) {
        lastLogMs = now;
        Serial.printf("[VCU] 102: D0=%02X stand=%d d100m=%u | 201: spdRaw=%u odo=%.1f brake=%d | 400 D7=%u | "
                      "scale=%.5f (%.1f km) speed=%.1f trip=%.1f %s\n",
                      v.stateRaw, v.standBit, v.dist100m, v.speedRaw, v.odoRaw / 10.0f, v.brakeBit,
                      v.odo400, v.speedScale, v.calibKm, v.speedKmh, v.tripKm,
                      v.motionValid ? "ONLINE" : "OFFLINE");
    }
}

const VcuData &vcuData() { return v; }

void vcuResetTrip() {
    if (!haveOdo) return;
    tripStartRaw = v.odoRaw;
    tripStartSet = true;
    prefs.putUInt("trip0", tripStartRaw);
    Serial.println("[VCU] reset TRIP");
}

void vcuDebugText(char *buf, size_t size) {
    const uint32_t now = millis();
    const bool s102 = fresh(last102Ms, now), s201 = fresh(last201Ms, now);
    char scale[24];
    if (v.calibrated) snprintf(scale, sizeof(scale), "%.5f", v.speedScale);
    else if (calRunning) snprintf(scale, sizeof(scale), "đo %.1f/%.1f km", (v.odoRaw - calStartOdo) / 10.0f, MIN_CALIB_KM);
    else snprintf(scale, sizeof(scale), "chờ ODO tăng");
    snprintf(buf, size,
             "102 %s D0=%02X chống=%d\n"
             "100m=%u  400 D7=%u\n"
             "201 %s v=%u (%04X)\n"
             "ODO=%u  phanh=%d\n"
             "hệ số: %s",
             s102 ? "OK" : "--", v.stateRaw, v.standBit, v.dist100m, v.odo400,
             s201 ? "OK" : "--", v.speedRaw, v.speedRaw, v.odoRaw, v.brakeBit, scale);
}
