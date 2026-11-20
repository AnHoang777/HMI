#pragma once
// Các font được tạo bởi tools/gen_fonts.sh (Be Vietnam Pro)
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

LV_FONT_DECLARE(font_speed_120)   // số tốc độ
LV_FONT_DECLARE(font_soc_54)      // SOC %
LV_FONT_DECLARE(font_range_46)    // quãng đường còn lại
LV_FONT_DECLARE(font_value_32)    // giá trị số
LV_FONT_DECLARE(font_mid_24)      // đơn vị, logo, chế độ lái
LV_FONT_DECLARE(font_label_18)    // nhãn tiếng Việt

#ifdef __cplusplus
}
#endif
