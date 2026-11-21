#pragma once
// Ước tính quãng đường còn đi được = SOC × (km đi được cho mỗi 1 % pin).
// Số km/% tự học khi xe chạy: mỗi lần SOC giảm 1 %, lấy quãng đường ODO đã đi được.
// Không dùng dòng điện (hệ số dòng của 0x309 giữ theo code_sac, chưa kiểm chứng khi chạy).
// Hệ số đã học lưu trong NVS, giữ qua các lần tắt máy.
#include <stdint.h>

void rangeBegin();
// Gọi định kỳ với dữ liệu mới nhất. odoRaw đơn vị 0,1 km.
void rangeUpdate(bool socValid, float soc, bool odoValid, uint16_t odoRaw);
float rangeKm();          // NAN khi chưa có SOC
float rangeKmPerPct();    // hệ số đang dùng
