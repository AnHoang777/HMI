#pragma once
#include "vehicle.h"

typedef void (*ModeTapHandler)();

// Tạo giao diện bảng đồng hồ 800x480
void uiCreate(ModeTapHandler onModeTap);

// Cập nhật số liệu
void uiUpdate(const VehicleState &s);
