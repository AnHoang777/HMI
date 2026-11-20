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

// Icon Material Symbols Rounded
LV_FONT_DECLARE(font_icons_28)    // speed, route, thermometer, bolt, power, offline_bolt
LV_FONT_DECLARE(font_icons_20)    // bluetooth, eco
LV_FONT_DECLARE(font_icon_96)     // electric_moped

#define ICON_ODO          0xE9E4
#define ICON_TRIP         0xEACD
#define ICON_THERMOMETER  0xE846
#define ICON_BOLT         0xEA0B
#define ICON_PLUG         0xE63C
#define ICON_BOLT_CIRCLE  0xE932
#define ICON_BLUETOOTH    0xE1A7
#define ICON_ECO          0xEA35
#define ICON_SCOOTER      0xEB1D

#ifdef __cplusplus
}
#endif
