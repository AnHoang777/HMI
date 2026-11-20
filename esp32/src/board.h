#pragma once
// Phần cứng Waveshare ESP32-S3-Touch-LCD-4.3:
//  - Màn RGB 800x480 (16 bit), framebuffer trong PSRAM
//  - Cảm ứng GT911 (I2C)
//  - Mở rộng IO CH422G (I2C): reset cảm ứng, bật đèn nền, reset LCD
#include <stdint.h>

constexpr int LCD_W = 800;
constexpr int LCD_H = 480;

void boardInit();
void boardDrawBitmap(int x1, int y1, int x2, int y2, const void *pixels);
bool boardReadTouch(uint16_t &x, uint16_t &y);
