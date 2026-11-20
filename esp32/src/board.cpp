#include "board.h"

#include <Arduino.h>
#include <Wire.h>
#include <string.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>

namespace {

constexpr int PIN_I2C_SDA = 8;
constexpr int PIN_I2C_SCL = 9;
constexpr int PIN_TOUCH_INT = 4;

// CH422G: ghi 0x01 vào địa chỉ 0x24 để bật chế độ output, ghi trạng thái chân vào 0x38
constexpr uint8_t CH422G_MODE = 0x24;
constexpr uint8_t CH422G_OUT = 0x38;
// Bit: EXIO1 = reset cảm ứng, EXIO2 = đèn nền, EXIO3 = reset LCD, EXIO5 = chọn USB
constexpr uint8_t EXIO_TOUCH_IN_RESET = 0x2C;
constexpr uint8_t EXIO_RUN = 0x2E;

constexpr uint8_t GT911_ADDR = 0x5D;
constexpr uint16_t GT911_STATUS = 0x814E;
constexpr uint16_t GT911_POINT1 = 0x8150;

esp_lcd_panel_handle_t panel = nullptr;

void i2cWriteByte(uint8_t addr, uint8_t value) {
    Wire.beginTransmission(addr);
    Wire.write(value);
    Wire.endTransmission();
}

bool gt911Read(uint16_t reg, uint8_t *buf, size_t len) {
    Wire.beginTransmission(GT911_ADDR);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xFF);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom(GT911_ADDR, (uint8_t)len) != len) return false;
    for (size_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

void gt911Write(uint16_t reg, uint8_t value) {
    Wire.beginTransmission(GT911_ADDR);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xFF);
    Wire.write(value);
    Wire.endTransmission();
}

void initPanel() {
    esp_lcd_rgb_panel_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.clk_src = LCD_CLK_SRC_DEFAULT;
    cfg.timings.pclk_hz = 16 * 1000 * 1000;
    cfg.timings.h_res = LCD_W;
    cfg.timings.v_res = LCD_H;
    cfg.timings.hsync_pulse_width = 4;
    cfg.timings.hsync_back_porch = 8;
    cfg.timings.hsync_front_porch = 8;
    cfg.timings.vsync_pulse_width = 4;
    cfg.timings.vsync_back_porch = 8;
    cfg.timings.vsync_front_porch = 8;
    cfg.timings.flags.pclk_active_neg = 1;

    cfg.data_width = 16;
    cfg.num_fbs = 1;
    cfg.bounce_buffer_size_px = LCD_W * 10;   // giảm nhiễu hình khi PSRAM bận
    cfg.psram_trans_align = 64;

    cfg.hsync_gpio_num = 46;
    cfg.vsync_gpio_num = 3;
    cfg.de_gpio_num = 5;
    cfg.pclk_gpio_num = 7;
    cfg.disp_gpio_num = -1;

    // B3..B7, G2..G7, R3..R7
    const int data[16] = {14, 38, 18, 17, 10, 39, 0, 45, 48, 47, 21, 1, 2, 42, 41, 40};
    for (int i = 0; i < 16; i++) cfg.data_gpio_nums[i] = data[i];

    cfg.flags.fb_in_psram = 1;

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&cfg, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
}

}  // namespace

void boardInit() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);

    // Reset GT911 với chân INT kéo thấp → địa chỉ I2C 0x5D
    i2cWriteByte(CH422G_MODE, 0x01);
    i2cWriteByte(CH422G_OUT, EXIO_TOUCH_IN_RESET);
    delay(100);
    pinMode(PIN_TOUCH_INT, OUTPUT);
    digitalWrite(PIN_TOUCH_INT, LOW);
    delay(100);
    i2cWriteByte(CH422G_OUT, EXIO_RUN);   // thả reset, bật đèn nền
    delay(200);
    pinMode(PIN_TOUCH_INT, INPUT);

    initPanel();
}

void boardDrawBitmap(int x1, int y1, int x2, int y2, const void *pixels) {
    esp_lcd_panel_draw_bitmap(panel, x1, y1, x2 + 1, y2 + 1, pixels);
}

bool boardReadTouch(uint16_t &x, uint16_t &y) {
    static bool pressed = false;
    static uint16_t lastX = 0, lastY = 0;

    uint8_t status = 0;
    if (gt911Read(GT911_STATUS, &status, 1) && (status & 0x80)) {
        uint8_t points = status & 0x0F;
        if (points > 0) {
            uint8_t buf[4];
            if (gt911Read(GT911_POINT1, buf, sizeof(buf))) {
                lastX = buf[0] | (buf[1] << 8);
                lastY = buf[2] | (buf[3] << 8);
                pressed = true;
            }
        } else {
            pressed = false;
        }
        gt911Write(GT911_STATUS, 0);
    }
    x = lastX;
    y = lastY;
    return pressed;
}
