#pragma once
// Chi tiết cắt từ ảnh thiết kế (esp32/assets/design.jpg) – tạo bởi tools/extract_assets.sh
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

LV_IMG_DECLARE(img_logo_small)    // logo V đầu màn
LV_IMG_DECLARE(img_logo_center)   // logo V + chữ VINFAST giữa ô dưới
LV_IMG_DECLARE(img_bluetooth)
LV_IMG_DECLARE(img_road)          // icon đường "A" (ODO, TRIP)
LV_IMG_DECLARE(img_thermo)
LV_IMG_DECLARE(img_bolt)          // tia sét trong vòng tròn
LV_IMG_DECLARE(img_plug)
LV_IMG_DECLARE(img_current)
LV_IMG_DECLARE(img_scooter)
LV_IMG_DECLARE(img_leaf_l)
LV_IMG_DECLARE(img_leaf_r)

#ifdef __cplusplus
}
#endif
