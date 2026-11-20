#pragma once
// Các font được tạo bởi tools/gen_fonts.sh (Be Vietnam Pro)
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

LV_FONT_DECLARE(font_speed_104)   // số tốc độ
LV_FONT_DECLARE(font_soc_44)      // SOC %
LV_FONT_DECLARE(font_range_44)    // quãng đường còn lại
LV_FONT_DECLARE(font_value_26)    // giá trị số
LV_FONT_DECLARE(font_mid_22)      // logo, đơn vị, chế độ lái, giờ
LV_FONT_DECLARE(font_label_16)    // nhãn tiếng Việt
LV_FONT_DECLARE(font_italic_16)   // khẩu hiệu cuối màn
#ifdef __cplusplus
}
#endif
