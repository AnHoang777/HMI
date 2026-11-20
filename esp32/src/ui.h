#pragma once
#include "vehicle.h"

typedef void (*ModeTapHandler)();

// Tạo giao diện bảng đồng hồ 800x480
void uiCreate(ModeTapHandler onModeTap);

// Cập nhật số liệu. liveData = true khi đang nhận dữ liệu thật từ xe.
void uiUpdate(const VehicleState &s, const char *statusText, bool liveData);
