#pragma once
// Đọc dữ liệu xe (VCU/MCU) trên cùng bus CAN với BMS – bảng ID do người dùng dò được:
//   0x102 D0      : trạng thái ECO / SPORT / P / READY (bảng giá trị chưa biết → MODE_TABLE trong vcu.cpp)
//   0x102 D1 bit4 : chân chống
//   0x102 D7      : bộ đếm tăng mỗi 100 m
//   0x201 D0–D1   : tốc độ dạng raw (Big Endian) – hệ số km/h tự hiệu chỉnh theo ODO
//   0x201 D4–D5   : ODO, Big Endian, /10 km
//   0x201 D7 bit4 : phanh
//   0x400 D7      : bộ đếm liên quan ODO
// TRIP = ODO hiện tại − ODO lúc reset (lưu trong NVS), giữ lâu vào ô TRIP để reset.
#include <math.h>
#include <stddef.h>
#include <stdint.h>

struct VcuData {
    bool stateValid = false;   // 0x102 còn mới
    bool motionValid = false;  // 0x201 còn mới

    uint8_t stateRaw = 0;      // 0x102 D0
    bool standBit = false;     // 0x102 D1 bit4
    uint8_t dist100m = 0;      // 0x102 D7
    uint16_t speedRaw = 0;     // 0x201 D0–D1
    uint16_t odoRaw = 0;       // 0x201 D4–D5 (đơn vị 0,1 km)
    bool brakeBit = false;     // 0x201 D7 bit4
    uint8_t odo400 = 0;        // 0x400 D7

    float speedKmh = NAN;      // NAN khi chưa hiệu chỉnh hoặc mất tín hiệu
    float odoKm = NAN;
    float tripKm = NAN;

    bool calibrated = false;   // đã có hệ số tốc độ
    float speedScale = 0;      // km/h trên mỗi đơn vị raw
    float calibKm = 0;         // quãng đường đã dùng để hiệu chỉnh
};

// Tên chế độ theo 0x102 D0. Trả nullptr khi mã chưa có trong bảng; isPark = true khi là P.
const char *vcuModeName(uint8_t raw, uint32_t *color, bool *isPark);

void vcuBegin();                                                    // đọc hệ số tốc độ + mốc TRIP từ NVS
void vcuDecode(uint32_t id, const uint8_t *d, uint8_t len, uint32_t now);   // bms.cpp gọi cho mọi khung chuẩn
void vcuUpdate();                                                   // gọi trong loop()
const VcuData &vcuData();
void vcuResetTrip();
void vcuDebugText(char *buf, size_t size);                         // vài dòng số raw để hiện trên màn
