#pragma once
// Đọc dữ liệu xe (VCU/MCU) trên cùng bus CAN với BMS. Đã đối chiếu với log thực tế (21/11/2026):
//   0x102 D0 bit4 : 1 = READY, 0 = P (xe chỉ chạy khi bit này bật)
//   0x102 D0 bit0 : 1 = SPORT, 0 = ECO (0x31 đạt 72 km/h, 0x30 tối đa ~52 km/h)
//   0x102 D1 bit4 : 1 = chân chống hạ (chỉ thấy khi xe đứng yên)
//   0x102 D7      : bộ đếm tăng mỗi 100 m
//   0x201 D0–D1   : tốc độ, Big Endian, /10 km/h (mỗi 100 m của D7 ứng với đúng 1,00 raw·giờ)
//   0x201 D4–D5   : ODO, Big Endian, /10 km (log: 8F3A → 8F3E sau 400 m)
//   0x201 D7 bit4 : 1 = bóp phanh
//   0x400 D6–D7   : bản sao ODO
// TRIP = ODO hiện tại − ODO lúc reset (lưu trong NVS), giữ lâu ô TRIP để reset.
#include <math.h>
#include <stddef.h>
#include <stdint.h>

struct VcuData {
    bool stateValid = false;   // 0x102 còn mới
    bool motionValid = false;  // 0x201 còn mới

    uint8_t stateRaw = 0;      // 0x102 D0
    bool ready = false;        // D0 bit4
    bool sport = false;        // D0 bit0
    bool standDown = false;    // 0x102 D1 bit4
    uint8_t dist100m = 0;      // 0x102 D7
    uint16_t speedRaw = 0;     // 0x201 D0–D1
    uint16_t odoRaw = 0;       // 0x201 D4–D5 (đơn vị 0,1 km)
    bool brake = false;        // 0x201 D7 bit4

    float speedKmh = NAN;      // NAN khi mất tín hiệu
    float odoKm = NAN;
    float tripKm = NAN;
};

void vcuBegin();                                                    // đọc mốc TRIP từ NVS
void vcuDecode(uint32_t id, const uint8_t *d, uint8_t len, uint32_t now);   // bms.cpp gọi cho mọi khung chuẩn
void vcuUpdate();                                                   // gọi trong loop()
const VcuData &vcuData();
void vcuResetTrip();
void vcuDebugText(char *buf, size_t size);                         // vài dòng số raw để hiện trên màn
