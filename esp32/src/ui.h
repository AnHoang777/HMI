#pragma once
#include "vehicle.h"

typedef void (*UiHandler)();

struct UiCallbacks {
    UiHandler onModeTap = nullptr;    // chạm nút chế độ lái
    UiHandler onTripReset = nullptr;  // giữ lâu ô TRIP
};

// Trạng thái phụ lấy từ CAN của xe. -1 = chưa biết, 0 = tắt, 1 = bật
struct UiExtras {
    int8_t ready = -1;                // 1 = READY, 0 = P
    int8_t brake = -1;
    int8_t stand = -1;
    const char *modeText = nullptr;   // nullptr = dùng s.mode
    uint32_t modeColor = 0x5A6478;
    const char *debug = nullptr;      // các dòng raw hiện trong ô hình xe (chạm ô để bật/tắt)
    const char *warning = nullptr;    // cảnh báo đỏ nhấp nháy giữa thanh tiêu đề, nullptr = ẩn
};

// Tạo giao diện bảng đồng hồ 800x480
void uiCreate(const UiCallbacks &cb);

// Cập nhật số liệu. bmsConnected = true khi đang nhận dữ liệu thật từ BMS.
void uiUpdate(const VehicleState &s, bool bmsConnected, const UiExtras &x = UiExtras());
