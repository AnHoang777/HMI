#pragma once
// Đọc BMS qua CAN (TWAI) – cùng bảng giải mã với firmware sạc code_sac.ino
//   0x309: điện áp pack (byte 2-3, 0.002 V/count), dòng điện (byte 6-7, int16, 0.02 A/count, dương = sạc)
//   0x30A: SOC (byte 2 với pack 20 cell, byte 1 với pack 19 cell), SOH (byte 3, chưa kiểm chứng)
//   0x320: nhiệt độ T1 (byte 0), T2 = nhiệt độ pin (byte 1), T3 (byte 3), đơn vị °C
//   0x31A: byte 6-7 hợp lệ (2–4 V) 3 lần liên tiếp → pack 20 cell
// Chỉ nghe, không gửi lệnh nào lên bus (TWAI vẫn ở chế độ NORMAL để ACK khung của BMS).
// Mọi khung nhận được cũng được chuyển cho vcu.cpp để đọc tốc độ, ODO, phanh, chế độ.
#include <stdint.h>

struct BmsData {
    bool packValid = false;    // điện áp + dòng điện
    bool socValid = false;
    bool sohValid = false;
    bool tempValid = false;

    float voltage = 0;         // V
    float current = 0;         // A, dương = sạc, âm = xả
    float soc = 0;             // %
    float soh = 0;             // %
    float temps[3] = {};       // °C: [0] T1, [1] nhiệt độ pin (giống màn sạc), [2] T3
    uint8_t cellCount = 19;

    float batteryTemp() const { return temps[1]; }
    float maxTemp() const;
};

void bmsBegin();
void bmsPoll();                 // gọi thường xuyên trong loop()
bool bmsOnline();               // có khung BMS trong 1,2 s gần nhất
const BmsData &bmsData();       // các cờ *Valid đã tính cả thời gian hết hạn
