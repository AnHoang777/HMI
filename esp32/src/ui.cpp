// Giao diện bảng đồng hồ neon – LVGL 8.3, màn 800x480, bám theo ảnh thiết kế esp32/assets/design.jpg
//
//  ┌ 📅 20/11/2026  12:51 ─────────────────────────── ᛒ ● Kết nối BMS ┐
//  │ [pin dọc] SOC 89%  ╲      ( đồng hồ tròn )      ╱  (A) ODO      │
//  │ [ xe tay ga ]      ╱      24 km/h  [ECO]        ╲  (A) TRIP     │
//  │                                                    🌡 Nhiệt độ pin │
//  │ ⚡ Ước tính còn đi được 71 km │ [P] (READY) (!) │ 🔌 Điện áp / ⚡ Dòng điện │
//  └──────────────────────────────────────────────────────────────────┘
//
// Lớp nền (khung neon, ô thẻ góc vát, vòng đồng hồ, vạch chia) vẽ trực tiếp.
// Logo, icon, hình xe là ảnh cắt từ thiết kế. Số liệu, cung tốc độ, cột pin là widget LVGL.
#include "ui.h"

#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "fonts/fonts.h"
#include "images/images.h"

namespace {

constexpr int W = 800;
constexpr int H = 480;

struct Box {
    int x, y, w, h;
    int cy() const { return y + h / 2; }
    int cx() const { return x + w / 2; }
};

// Góc vát (px) cho từng góc: trên-trái, trên-phải, dưới-phải, dưới-trái
struct Cut {
    int tl, tr, br, bl;
};

// Các ô thẻ
constexpr Box CARD_SOC{12, 66, 238, 130};
constexpr Box CARD_BIKE{12, 204, 238, 124};
constexpr Box CARD_R[3] = {{550, 66, 238, 82}, {550, 156, 238, 82}, {550, 246, 238, 82}};
constexpr Box CARD_BOTTOM{12, 338, 776, 116};

constexpr Cut CUT_SOC{8, 26, 8, 8};
constexpr Cut CUT_BIKE{8, 30, 14, 8};
constexpr Cut CUT_R[3] = {{26, 8, 8, 8}, {14, 8, 8, 14}, {8, 8, 8, 26}};
constexpr Cut CUT_BOTTOM{14, 14, 14, 14};

// Đồng hồ tốc độ
constexpr int GX = 400;
constexpr int GY = 198;
constexpr int G_R = 116;      // bán kính đường giữa cung
constexpr int G_W = 14;       // độ dày cung
constexpr int G_RING = 134;   // vòng neon ngoài
constexpr int TICK_R = 95;
constexpr int G_START = 135;  // độ, 0 = bên phải, chiều kim đồng hồ
constexpr int G_SWEEP = 270;
constexpr int MAX_SPEED = 80;

// Cột pin dọc: 5 vạch, dưới đỏ → trên xanh lá, mỗi vạch 20%
constexpr int BATT_X = 34, BATT_Y = 88, BATT_W = 46, BATT_H = 98;
constexpr int NUB_W = 22, NUB_H = 7;
constexpr uint32_t SEG_COLORS[5] = {0xFF3B3B, 0xFF8A1C, 0xFFD21C, 0xA6F23D, 0x3DF26A};

// Cột phải: icon & chữ
constexpr int R_ICON_X = 590;
constexpr int R_TEXT_X = 630;

// Thanh tiêu đề: dải giữa khung neon trên (y = 6) và vạch tiêu đề hai bên (y = 58)
constexpr int HEADER_CY = 32;
constexpr int HEADER_SIDE = 26;   // lề trái / phải bằng nhau

// Ô dưới: quãng đường còn lại | đèn báo P / READY / (!) | điện áp, dòng điện
constexpr int DIV_X[2] = {262, 539};
constexpr int IND_Y = 396;
constexpr int IND_X[3] = {309, 409, 501};   // P, READY, (!)
constexpr int VA_ICON_X = 585;
constexpr int VA_TEXT_X = 614;
constexpr int VA_ROW_Y[2] = {362, 414};

inline lv_color_t cCyan() { return lv_color_hex(0x19E0FF); }
inline lv_color_t cBlue() { return lv_color_hex(0x1E6BFF); }
inline lv_color_t cGreen() { return lv_color_hex(0x3DF26A); }
inline lv_color_t cWhite() { return lv_color_white(); }
inline lv_color_t cLabel() { return lv_color_hex(0x5FC8FF); }

// Màu nền (gradient dọc) tại toạ độ y – dùng để trộn màu "phát sáng" không cần độ trong suốt
lv_color_t bgAt(int y) {
    if (y < 0) y = 0;
    if (y > H) y = H;
    return lv_color_mix(lv_color_hex(0x040912), lv_color_hex(0x0A1630), (uint8_t)(y * 255 / H));
}

// ---------------------------------------------------------------- hình học tĩnh
// Khung trên vát xuống ở giữa; hai bên đủ rộng cho cụm ngày giờ (trái) và "Kết nối BMS" (phải)
lv_point_t topPts[6] = {{12, 6}, {290, 6}, {308, 24}, {492, 24}, {510, 6}, {788, 6}};
lv_point_t headerPts[6] = {{12, 58}, {240, 58}, {262, 48}, {538, 48}, {560, 58}, {788, 58}};
lv_point_t footPts[4] = {{8, 452}, {24, 468}, {776, 468}, {792, 452}};

// ---------------------------------------------------------------- vẽ cơ bản
void drawLine(lv_draw_ctx_t *ctx, lv_color_t c, int w, int x1, int y1, int x2, int y2) {
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color = c;
    d.width = w;
    d.round_start = 1;
    d.round_end = 1;
    lv_point_t p1 = {(lv_coord_t)x1, (lv_coord_t)y1};
    lv_point_t p2 = {(lv_coord_t)x2, (lv_coord_t)y2};
    lv_draw_line(ctx, &d, &p1, &p2);
}

// Đường neon 3 lớp: quầng rộng mờ → quầng vừa → lõi sáng. Xanh lam ở hai đầu, cyan ở giữa.
void neonPolyline(lv_draw_ctx_t *ctx, const lv_point_t *p, int n, int width, bool fadeEnds) {
    const int widths[3] = {width + 8, width + 3, width};
    const uint8_t strength[3] = {45, 110, 255};
    for (int pass = 0; pass < 3; pass++) {
        for (int i = 0; i < n - 1; i++) {
            float t = (p[i].x + p[i + 1].x) / 2.0f / W;
            float centerness = 1 - fabsf(t - 0.5f) * 2;
            lv_color_t c = lv_color_mix(cCyan(), cBlue(), (uint8_t)(centerness * 255));
            if (pass == 2) c = lv_color_mix(cWhite(), c, 70);
            float fade = fadeEnds ? fminf(1.0f, fminf(t, 1 - t) * 8) : 1.0f;
            uint8_t mix = (uint8_t)(strength[pass] * fade);
            int midY = (p[i].y + p[i + 1].y) / 2;
            drawLine(ctx, lv_color_mix(c, bgAt(midY), mix), widths[pass],
                     p[i].x, p[i].y, p[i + 1].x, p[i + 1].y);
        }
    }
}

void drawCircle(lv_draw_ctx_t *ctx, int cx, int cy, int r, lv_color_t c, lv_opa_t opa) {
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.radius = LV_RADIUS_CIRCLE;
    d.bg_color = c;
    d.bg_opa = opa;
    lv_area_t a = {(lv_coord_t)(cx - r), (lv_coord_t)(cy - r), (lv_coord_t)(cx + r), (lv_coord_t)(cy + r)};
    lv_draw_rect(ctx, &d, &a);
}

void drawArc(lv_draw_ctx_t *ctx, int cx, int cy, int r, int w, lv_color_t c, int a0, int a1, bool rounded) {
    lv_draw_arc_dsc_t d;
    lv_draw_arc_dsc_init(&d);
    d.color = c;
    d.width = w;
    d.rounded = rounded ? 1 : 0;
    lv_point_t center = {(lv_coord_t)cx, (lv_coord_t)cy};
    lv_draw_arc(ctx, &d, &center, r, a0, a1);
}

void drawRing(lv_draw_ctx_t *ctx, int cx, int cy, int r, int w, lv_color_t c) {
    drawArc(ctx, cx, cy, r, w, c, 0, 360, false);
}

// Ô thẻ góc vát: nền gradient + viền neon mảnh có quầng sáng
int cardPolygon(const Box &b, const Cut &c, lv_point_t *p) {
    const int x0 = b.x, y0 = b.y, x1 = b.x + b.w - 1, y1 = b.y + b.h - 1;
    int n = 0;
    auto add = [&](int x, int y) { p[n].x = (lv_coord_t)x; p[n].y = (lv_coord_t)y; n++; };
    add(x0, y0 + c.tl); add(x0 + c.tl, y0);
    add(x1 - c.tr, y0); add(x1, y0 + c.tr);
    add(x1, y1 - c.br); add(x1 - c.br, y1);
    add(x0 + c.bl, y1); add(x0, y1 - c.bl);
    return n;
}

void drawCard(lv_draw_ctx_t *ctx, const Box &b, const Cut &c) {
    lv_point_t p[9];
    int n = cardPolygon(b, c, p);

    lv_draw_rect_dsc_t fill;
    lv_draw_rect_dsc_init(&fill);
    fill.bg_opa = LV_OPA_COVER;
    fill.bg_color = lv_color_hex(0x0E2446);
    fill.bg_grad.dir = LV_GRAD_DIR_VER;
    fill.bg_grad.stops_count = 2;
    fill.bg_grad.stops[0].color = lv_color_hex(0x0E2446);
    fill.bg_grad.stops[0].frac = 0;
    fill.bg_grad.stops[1].color = lv_color_hex(0x071328);
    fill.bg_grad.stops[1].frac = 255;
    lv_draw_polygon(ctx, &fill, p, n);

    p[n] = p[0];   // khép kín viền
    const lv_color_t edge = lv_color_hex(0x2F7BFF);
    const int widths[3] = {7, 3, 2};
    for (int pass = 0; pass < 3; pass++) {
        for (int i = 0; i < n; i++) {
            int midY = (p[i].y + p[i + 1].y) / 2;
            lv_color_t col = pass == 0 ? lv_color_mix(edge, bgAt(midY), 40)
                           : pass == 1 ? lv_color_mix(edge, bgAt(midY), 110)
                                       : lv_color_mix(cWhite(), edge, 40);
            drawLine(ctx, col, widths[pass], p[i].x, p[i].y, p[i + 1].x, p[i + 1].y);
        }
    }
}

// ---------------------------------------------------------------- lớp nền
void drawBack(lv_event_t *e) {
    lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);

    // khung neon trên / dưới
    neonPolyline(ctx, topPts, 6, 2, true);
    neonPolyline(ctx, headerPts, 6, 3, true);
    neonPolyline(ctx, footPts, 4, 2, false);

    // quầng sáng lớn quanh đồng hồ (dưới các ô thẻ)
    drawCircle(ctx, GX, GY, G_RING + 40, cBlue(), 16);
    drawCircle(ctx, GX, GY, G_RING + 20, cBlue(), 22);

    // ô thẻ
    drawCard(ctx, CARD_SOC, CUT_SOC);
    drawCard(ctx, CARD_BIKE, CUT_BIKE);
    for (int i = 0; i < 3; i++) drawCard(ctx, CARD_R[i], CUT_R[i]);
    drawCard(ctx, CARD_BOTTOM, CUT_BOTTOM);

    // vạch chia trong ô dưới
    lv_draw_rect_dsc_t div;
    lv_draw_rect_dsc_init(&div);
    div.bg_opa = LV_OPA_COVER;
    div.bg_color = lv_color_hex(0x2A5AA8);
    for (int x : DIV_X) {
        lv_area_t divA = {(lv_coord_t)x, 356, (lv_coord_t)(x + 1), 436};
        lv_draw_rect(ctx, &div, &divA);
    }

    // đồng hồ: lòng tối, vòng neon ngoài nhiều lớp, vòng trong mảnh, rãnh cung, vạch chia
    const lv_color_t ringBg = bgAt(GY);
    drawCircle(ctx, GX, GY, G_RING - 2, lv_color_hex(0x040A18), 235);
    drawRing(ctx, GX, GY, G_RING + 12, 14, lv_color_mix(cBlue(), ringBg, 22));
    drawRing(ctx, GX, GY, G_RING + 7, 9, lv_color_mix(cBlue(), ringBg, 60));
    drawRing(ctx, GX, GY, G_RING + 3, 5, lv_color_mix(cBlue(), ringBg, 140));
    drawRing(ctx, GX, GY, G_RING, 3, lv_color_mix(cWhite(), lv_color_hex(0x3A8BFF), 70));
    drawRing(ctx, GX, GY, 102, 1, lv_color_mix(cBlue(), ringBg, 90));

    drawArc(ctx, GX, GY, G_R + G_W / 2 + 4, G_W + 8, lv_color_hex(0x0B1C3A),
            G_START, (G_START + G_SWEEP) % 360, true);
    drawArc(ctx, GX, GY, G_R + G_W / 2, G_W, lv_color_hex(0x132B55),
            G_START, (G_START + G_SWEEP) % 360, true);

    for (int i = 0; i <= 20; i++) {
        bool major = (i % 5) == 0;
        float ang = (G_START + (float)G_SWEEP * i / 20) * (float)M_PI / 180.0f;
        int len = major ? 14 : 7;
        float r0 = TICK_R - len / 2.0f, r1 = TICK_R + len / 2.0f;
        int x0 = GX + (int)lroundf(r0 * cosf(ang)), y0 = GY + (int)lroundf(r0 * sinf(ang));
        int x1 = GX + (int)lroundf(r1 * cosf(ang)), y1 = GY + (int)lroundf(r1 * sinf(ang));
        drawLine(ctx, lv_color_mix(cWhite(), bgAt(y0), major ? 200 : 80), major ? 3 : 2, x0, y0, x1, y1);
    }
}

// ---------------------------------------------------------------- widget
UiCallbacks callbacks;

lv_obj_t *speedLbl, *arcGlowWide, *arcGlow, *arcMain, *arcCore;
lv_obj_t *scooterImg, *debugLbl;
bool debugShown = false;

// Đèn báo giữa ô dưới: P, READY, (!) phanh
struct Badge {
    lv_obj_t *box, *lbl;
    uint32_t onColor;
    int state;
};
Badge badgePark, badgeReady, badgeBrake;
lv_obj_t *dateLbl;
lv_obj_t *warnBox, *warnLbl;   // cảnh báo mất tín hiệu giữa thanh tiêu đề
lv_obj_t *socLbl, *battBox, *battNub, *battSeg[5];
lv_obj_t *odoLbl, *tripLbl, *tempLbl;
lv_obj_t *rangeLbl, *voltLbl, *currLbl;
lv_obj_t *modeBtn, *modeLbl;
lv_obj_t *bmsDot, *clockLbl;

int lastBattLevel = -1;
char lastModeText[16] = "";
uint32_t lastModeColor = 0xFFFFFFFF;
int lastBms = -1;
uint32_t buildSeconds = 0;

lv_obj_t *makeLabel(lv_obj_t *parent, const lv_font_t *f, lv_color_t c, const char *text) {
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, c, 0);
    lv_label_set_text(l, text);
    return l;
}

void placeCenter(lv_obj_t *o, int cx, int cy) { lv_obj_align(o, LV_ALIGN_CENTER, cx - W / 2, cy - H / 2); }
void placeLeft(lv_obj_t *o, int x, int cy) { lv_obj_align(o, LV_ALIGN_LEFT_MID, x, cy - H / 2); }

lv_obj_t *makeBox(lv_obj_t *parent) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return o;
}

lv_obj_t *makeImage(lv_obj_t *parent, const lv_img_dsc_t *src, int cx, int cy) {
    lv_obj_t *img = lv_img_create(parent);
    lv_img_set_src(img, src);
    placeCenter(img, cx, cy);
    return img;
}

// Hàng "giá trị + đơn vị" căn theo đáy
lv_obj_t *makeValueRow(lv_obj_t *parent, const lv_font_t *fv, const lv_font_t *fu, lv_color_t c,
                       const char *unit, int unitLift, lv_obj_t **valueOut) {
    lv_obj_t *row = makeBox(parent);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(row, 6, 0);
    *valueOut = makeLabel(row, fv, c, "--");
    lv_obj_t *u = makeLabel(row, fu, c, unit);
    lv_obj_set_style_pad_bottom(u, unitLift, 0);
    return row;
}

// Nhãn nhỏ phía trên + hàng giá trị phía dưới
void makeStat(lv_obj_t *parent, int x, int cy, const char *title, lv_color_t titleColor,
              const char *unit, lv_obj_t **valueOut) {
    lv_obj_t *t = makeLabel(parent, &font_label_16, titleColor, title);
    placeLeft(t, x, cy - 13);
    lv_obj_t *r = makeValueRow(parent, &font_value_26, &font_label_16, cWhite(), unit, 3, valueOut);
    placeLeft(r, x, cy + 11);
}

lv_obj_t *makeArc(lv_obj_t *parent, int width, lv_color_t color, lv_opa_t opa) {
    lv_obj_t *a = lv_arc_create(parent);
    int outer = G_R + width / 2;
    lv_obj_set_size(a, outer * 2, outer * 2);
    placeCenter(a, GX, GY);
    lv_arc_set_rotation(a, G_START);
    lv_arc_set_bg_angles(a, 0, G_SWEEP);
    lv_arc_set_range(a, 0, MAX_SPEED * 10);
    lv_arc_set_value(a, 0);
    lv_obj_remove_style(a, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_pad_all(a, 0, 0);
    lv_obj_set_style_bg_opa(a, LV_OPA_TRANSP, 0);
    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(a, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(a, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(a, opa, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(a, true, LV_PART_INDICATOR);
    return a;
}

void setText(lv_obj_t *l, const char *t) {
    if (strcmp(lv_label_get_text(l), t) != 0) lv_label_set_text(l, t);
}

void onModeClicked(lv_event_t *) {
    if (callbacks.onModeTap) callbacks.onModeTap();
}

void onTripLongPress(lv_event_t *) {
    if (callbacks.onTripReset) callbacks.onTripReset();
}

void onBikeClicked(lv_event_t *) {
    debugShown = !debugShown;
}

// Vùng chạm trong suốt phủ lên một ô thẻ
lv_obj_t *makeTouchArea(lv_obj_t *parent, const Box &b) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(o, b.x, b.y);
    lv_obj_set_size(o, b.w, b.h);
    return o;
}

// Đèn báo kiểu viền neon: w x h, radius; chữ font đậm
Badge makeBadge(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t onColor,
                int cx, int cy, int w, int h, int radius) {
    Badge b;
    b.onColor = onColor;
    b.state = -2;
    b.box = makeBox(parent);
    lv_obj_set_size(b.box, w, h);
    lv_obj_set_style_radius(b.box, radius, 0);
    lv_obj_set_style_border_width(b.box, 3, 0);
    lv_obj_set_style_bg_opa(b.box, LV_OPA_COVER, 0);
    b.lbl = makeLabel(b.box, font, cWhite(), text);
    lv_obj_center(b.lbl);
    placeCenter(b.box, cx, cy);
    return b;
}

// 1 = sáng (viền + chữ màu, quầng sáng), 0 = tắt, -1 = chưa có dữ liệu (đều mờ)
void setBadge(Badge &b, int state) {
    if (state == b.state) return;
    b.state = state;
    const lv_color_t on = lv_color_hex(b.onColor);
    const lv_color_t base = lv_color_hex(0x07101F);
    if (state == 1) {
        lv_obj_set_style_bg_color(b.box, lv_color_mix(on, base, 40), 0);
        lv_obj_set_style_border_color(b.box, on, 0);
        lv_obj_set_style_shadow_color(b.box, on, 0);
        lv_obj_set_style_shadow_width(b.box, 18, 0);
        lv_obj_set_style_shadow_opa(b.box, 140, 0);
        lv_obj_set_style_text_color(b.lbl, on, 0);
    } else {
        lv_obj_set_style_bg_color(b.box, base, 0);
        lv_obj_set_style_border_color(b.box, state == 0 ? lv_color_mix(on, base, 60) : lv_color_hex(0x2A3550), 0);
        lv_obj_set_style_shadow_width(b.box, 0, 0);
        lv_obj_set_style_text_color(b.lbl, state == 0 ? lv_color_mix(on, base, 80) : lv_color_hex(0x3A4560), 0);
    }
}

// Icon lịch nhỏ vẽ bằng khối: 2 móc treo + khung + dải trên + 4 ô ngày
lv_obj_t *makeCalendarIcon(lv_obj_t *parent) {
    const lv_color_t c = lv_color_hex(0x5FC8FF);
    lv_obj_t *icon = makeBox(parent);
    lv_obj_set_size(icon, 24, 25);
    lv_obj_t *frame = makeBox(icon);
    lv_obj_set_pos(frame, 0, 4);
    lv_obj_set_size(frame, 24, 21);
    lv_obj_set_style_radius(frame, 4, 0);
    lv_obj_set_style_border_width(frame, 2, 0);
    lv_obj_set_style_border_color(frame, c, 0);
    lv_obj_t *band = makeBox(icon);
    lv_obj_set_pos(band, 0, 4);
    lv_obj_set_size(band, 24, 7);
    lv_obj_set_style_radius(band, 3, 0);
    lv_obj_set_style_bg_color(band, c, 0);
    lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
    for (int i = 0; i < 2; i++) {
        lv_obj_t *hook = makeBox(icon);
        lv_obj_set_pos(hook, 6 + i * 9, 0);
        lv_obj_set_size(hook, 3, 7);
        lv_obj_set_style_radius(hook, 1, 0);
        lv_obj_set_style_bg_color(hook, c, 0);
        lv_obj_set_style_bg_opa(hook, LV_OPA_COVER, 0);
    }
    for (int i = 0; i < 4; i++) {
        lv_obj_t *day = makeBox(icon);
        lv_obj_set_pos(day, 5 + (i % 2) * 9, 13 + (i / 2) * 5);
        lv_obj_set_size(day, 5, 3);
        lv_obj_set_style_bg_color(day, c, 0);
        lv_obj_set_style_bg_opa(day, LV_OPA_COVER, 0);
    }
    return icon;
}

// Ngày giờ: chưa có nguồn giờ thật (RTC / WiFi / điện thoại) nên tính từ lúc build firmware (UTC+7)
int buildDay = 1, buildMonth = 1, buildYear = 2026;

void parseBuildTime() {
    const char *t = __TIME__;   // "HH:MM:SS" (UTC trên GitHub Actions)
    int h = (t[0] - '0') * 10 + (t[1] - '0');
    int m = (t[3] - '0') * 10 + (t[4] - '0');
    int s = (t[6] - '0') * 10 + (t[7] - '0');
    buildSeconds = (h + 7) * 3600 + m * 60 + s;   // có thể vượt 24 h → sang ngày hôm sau

    const char *d = __DATE__;   // "Mmm dd yyyy"
    static const char *MONTHS = "JanFebMarAprMayJunJulAugSepOctNovDec";
    for (int i = 0; i < 12; i++) {
        if (strncmp(d, MONTHS + i * 3, 3) == 0) buildMonth = i + 1;
    }
    buildDay = (d[4] == ' ' ? 0 : d[4] - '0') * 10 + (d[5] - '0');
    buildYear = atoi(d + 7);
}

void dateAfterDays(int days, int &dd, int &mm, int &yy) {
    static const int DIM[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    dd = buildDay;
    mm = buildMonth;
    yy = buildYear;
    while (days-- > 0) {
        const bool leap = (yy % 4 == 0 && yy % 100 != 0) || yy % 400 == 0;
        const int dim = DIM[mm - 1] + (mm == 2 && leap ? 1 : 0);
        if (++dd > dim) {
            dd = 1;
            if (++mm > 12) {
                mm = 1;
                yy++;
            }
        }
    }
}

}  // namespace

void uiCreate(const UiCallbacks &cb) {
    callbacks = cb;
    parseBuildTime();

    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A1630), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x040912), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // --- lớp nền
    lv_obj_t *back = makeBox(scr);
    lv_obj_set_size(back, W, H);
    lv_obj_set_pos(back, 0, 0);
    lv_obj_add_event_cb(back, drawBack, LV_EVENT_DRAW_MAIN, NULL);

    // --- đầu màn: lịch + ngày + giờ | trạng thái BMS
    lv_obj_t *when = makeBox(scr);
    lv_obj_set_size(when, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(when, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(when, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(when, 10, 0);
    lv_obj_t *cal = makeCalendarIcon(when);
    lv_obj_set_style_translate_y(cal, -2, 0);   // chữ số nằm cao hơn tâm dòng → nâng icon cho thẳng hàng
    dateLbl = makeLabel(when, &font_mid_22, cWhite(), "--/--/----");
    clockLbl = makeLabel(when, &font_mid_22, cWhite(), "--:--");
    lv_obj_set_style_pad_left(clockLbl, 10, 0);
    placeLeft(when, HEADER_SIDE, HEADER_CY);

    lv_obj_t *status = makeBox(scr);
    lv_obj_set_size(status, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(status, 10, 0);
    lv_obj_t *bt = lv_img_create(status);
    lv_img_set_src(bt, &img_bluetooth);
    bmsDot = makeBox(status);
    lv_obj_set_size(bmsDot, 10, 10);
    lv_obj_set_style_radius(bmsDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(bmsDot, LV_OPA_COVER, 0);
    lv_obj_set_style_outline_width(bmsDot, 3, 0);
    lv_obj_set_style_outline_opa(bmsDot, 70, 0);
    makeLabel(status, &font_label_16, lv_color_hex(0xDCE6F5), "Kết nối BMS");
    lv_obj_align(status, LV_ALIGN_RIGHT_MID, -HEADER_SIDE, HEADER_CY - H / 2);

    // cảnh báo mất tín hiệu: viên thuốc đỏ "(!) MẤT TÍN HIỆU ..." nằm giữa, ẩn khi bình thường
    warnBox = makeBox(scr);
    lv_obj_set_size(warnBox, LV_SIZE_CONTENT, 24);
    lv_obj_set_style_pad_hor(warnBox, 12, 0);
    lv_obj_set_style_radius(warnBox, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(warnBox, lv_color_hex(0x3A0A12), 0);
    lv_obj_set_style_bg_opa(warnBox, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(warnBox, 2, 0);
    lv_obj_set_style_border_color(warnBox, lv_color_hex(0xFF3B3B), 0);
    lv_obj_set_style_shadow_color(warnBox, lv_color_hex(0xFF3B3B), 0);
    lv_obj_set_style_shadow_width(warnBox, 14, 0);
    lv_obj_set_style_shadow_opa(warnBox, 150, 0);
    lv_obj_set_flex_flow(warnBox, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(warnBox, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(warnBox, 8, 0);
    lv_obj_t *warnIcon = makeBox(warnBox);
    lv_obj_set_size(warnIcon, 16, 16);
    lv_obj_set_style_radius(warnIcon, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(warnIcon, lv_color_hex(0xFF3B3B), 0);
    lv_obj_set_style_bg_opa(warnIcon, LV_OPA_COVER, 0);
    lv_obj_t *bang = makeLabel(warnIcon, &font_label_16, cWhite(), "!");
    lv_obj_center(bang);
    warnLbl = makeLabel(warnBox, &font_label_16, lv_color_hex(0xFFD0D6), "");
    placeCenter(warnBox, GX, 36);
    lv_obj_add_flag(warnBox, LV_OBJ_FLAG_HIDDEN);

    // --- ô SOC: cột pin dọc 5 vạch + SOC %
    battNub = makeBox(scr);
    lv_obj_set_pos(battNub, BATT_X + (BATT_W - NUB_W) / 2, BATT_Y - NUB_H);
    lv_obj_set_size(battNub, NUB_W, NUB_H + 2);
    lv_obj_set_style_radius(battNub, 2, 0);
    lv_obj_set_style_bg_opa(battNub, LV_OPA_COVER, 0);

    battBox = makeBox(scr);
    lv_obj_set_pos(battBox, BATT_X, BATT_Y);
    lv_obj_set_size(battBox, BATT_W, BATT_H);
    lv_obj_set_style_border_width(battBox, 3, 0);
    lv_obj_set_style_radius(battBox, 8, 0);
    lv_obj_set_style_outline_width(battBox, 3, 0);
    lv_obj_set_style_outline_opa(battBox, 60, 0);
    lv_obj_set_style_bg_color(battBox, lv_color_hex(0x07101F), 0);
    lv_obj_set_style_bg_opa(battBox, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(battBox, 4, 0);
    lv_obj_set_style_pad_row(battBox, 3, 0);
    lv_obj_set_flex_flow(battBox, LV_FLEX_FLOW_COLUMN_REVERSE);   // vạch 0 (đỏ) ở dưới cùng
    for (int i = 0; i < 5; i++) {
        battSeg[i] = makeBox(battBox);
        lv_obj_set_width(battSeg[i], lv_pct(100));
        lv_obj_set_flex_grow(battSeg[i], 1);
        lv_obj_set_style_radius(battSeg[i], 3, 0);
        lv_obj_set_style_bg_color(battSeg[i], lv_color_hex(SEG_COLORS[i]), 0);
        lv_obj_set_style_bg_opa(battSeg[i], LV_OPA_COVER, 0);
    }

    lv_obj_t *socTitle = makeLabel(scr, &font_mid_22, cWhite(), "SOC");
    placeLeft(socTitle, 104, 106);
    socLbl = makeLabel(scr, &font_soc_44, cWhite(), "--%");
    placeLeft(socLbl, 102, 150);

    // --- ô hình xe
    scooterImg = makeImage(scr, &img_scooter, CARD_BIKE.cx(), CARD_BIKE.cy() + 2);
    debugLbl = makeLabel(scr, &font_label_16, lv_color_hex(0x9FE8FF), "");
    lv_obj_set_style_text_line_space(debugLbl, -2, 0);
    lv_obj_set_pos(debugLbl, CARD_BIKE.x + 12, CARD_BIKE.y + 6);
    lv_obj_add_flag(debugLbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(makeTouchArea(scr, CARD_BIKE), onBikeClicked, LV_EVENT_CLICKED, NULL);

    // --- đồng hồ tốc độ: 3 lớp quầng + lõi sáng
    arcGlowWide = makeArc(scr, 36, cBlue(), 45);
    arcGlow = makeArc(scr, 24, cBlue(), 120);
    arcMain = makeArc(scr, G_W, lv_color_hex(0x22C8FF), LV_OPA_COVER);
    arcCore = makeArc(scr, 4, lv_color_hex(0xBFF4FF), 170);

    speedLbl = makeLabel(scr, &font_speed_104, cWhite(), "0");
    placeCenter(speedLbl, GX, 188);

    lv_obj_t *kmh = makeLabel(scr, &font_mid_22, cWhite(), "km/h");
    placeCenter(kmh, GX, 246);

    modeBtn = lv_obj_create(scr);
    lv_obj_remove_style_all(modeBtn);
    lv_obj_clear_flag(modeBtn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(modeBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(modeBtn, 104, 34);
    lv_obj_set_style_radius(modeBtn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(modeBtn, 2, 0);
    lv_obj_set_style_bg_opa(modeBtn, 35, 0);
    lv_obj_set_style_outline_width(modeBtn, 4, 0);
    lv_obj_set_style_outline_opa(modeBtn, 55, 0);
    lv_obj_set_ext_click_area(modeBtn, 20);
    lv_obj_add_event_cb(modeBtn, onModeClicked, LV_EVENT_CLICKED, NULL);
    modeLbl = makeLabel(modeBtn, &font_mid_22, cGreen(), "ECO");
    lv_obj_center(modeLbl);
    placeCenter(modeBtn, GX, 286);

    // --- ô bên phải
    makeImage(scr, &img_road, R_ICON_X, CARD_R[0].cy());
    makeImage(scr, &img_road, R_ICON_X, CARD_R[1].cy());
    makeImage(scr, &img_thermo, R_ICON_X, CARD_R[2].cy());
    makeStat(scr, R_TEXT_X, CARD_R[0].cy(), "ODO", cLabel(), "km", &odoLbl);
    makeStat(scr, R_TEXT_X, CARD_R[1].cy(), "TRIP", cLabel(), "km", &tripLbl);
    makeStat(scr, R_TEXT_X, CARD_R[2].cy(), "Nhiệt độ pin", lv_color_hex(0xDCE6F5), "°C", &tempLbl);
    lv_obj_add_event_cb(makeTouchArea(scr, CARD_R[1]), onTripLongPress, LV_EVENT_LONG_PRESSED, NULL);

    // --- ô dưới: quãng đường còn lại | P, READY, (!) | điện áp, dòng điện
    makeImage(scr, &img_bolt, 50, CARD_BOTTOM.cy());
    lv_obj_t *rangeTitle = makeLabel(scr, &font_label_16, lv_color_hex(0xDCE6F5), "Ước tính còn đi được");
    placeLeft(rangeTitle, 90, 374);
    lv_obj_t *rangeRow = makeValueRow(scr, &font_range_44, &font_mid_22, cGreen(), "km", 5, &rangeLbl);
    placeLeft(rangeRow, 90, 414);

    badgePark = makeBadge(scr, "P", &font_value_26, 0x3DF26A, IND_X[0], IND_Y, 46, 44, 9);
    badgeReady = makeBadge(scr, "READY", &font_mid_22, 0x3DF26A, IND_X[1], IND_Y, 108, 40, LV_RADIUS_CIRCLE);
    badgeBrake = makeBadge(scr, "!", &font_value_26, 0xFF3B3B, IND_X[2], IND_Y, 44, 44, LV_RADIUS_CIRCLE);

    makeImage(scr, &img_plug, VA_ICON_X, VA_ROW_Y[0]);
    makeImage(scr, &img_current, VA_ICON_X, VA_ROW_Y[1]);
    makeStat(scr, VA_TEXT_X, VA_ROW_Y[0], "Điện áp pin", cLabel(), "V", &voltLbl);
    makeStat(scr, VA_TEXT_X, VA_ROW_Y[1], "Dòng điện", cLabel(), "A", &currLbl);
}

// Giá trị NAN = chưa có dữ liệu → hiện "--"
static void setNum(lv_obj_t *lbl, const char *fmt, float v, bool asInt = false) {
    char buf[24];
    if (isnan(v)) {
        setText(lbl, "--");
        return;
    }
    if (asInt) snprintf(buf, sizeof(buf), fmt, (int)lroundf(v));
    else snprintf(buf, sizeof(buf), fmt, v);
    setText(lbl, buf);
}

void uiUpdate(const VehicleState &s, bool bmsConnected, const UiExtras &x) {
    char buf[32];

    // đèn báo
    // cảnh báo mất tín hiệu: nhấp nháy 0,5 s sáng / 0,5 s tắt
    if (x.warning) {
        setText(warnLbl, x.warning);
        if ((millis() / 500) % 2 == 0) lv_obj_clear_flag(warnBox, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(warnBox, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(warnBox, LV_OBJ_FLAG_HIDDEN);
    }

    // P sáng khi đỗ, READY sáng khi sẵn sàng chạy, (!) sáng khi bóp phanh
    setBadge(badgePark, x.ready < 0 ? -1 : (x.ready == 0 ? 1 : 0));
    setBadge(badgeReady, x.ready);
    setBadge(badgeBrake, x.brake);

    // ô raw thay cho hình xe (chạm ô để bật/tắt)
    const bool showDebug = debugShown && x.debug;
    if (showDebug) {
        setText(debugLbl, x.debug);
        lv_obj_clear_flag(debugLbl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(scooterImg, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(debugLbl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(scooterImg, LV_OBJ_FLAG_HIDDEN);
    }

    // tốc độ
    setNum(speedLbl, "%d", s.speed, true);
    const float spd = isnan(s.speed) ? 0 : s.speed;
    int arcVal = (int)lroundf(fminf(fmaxf(spd, 0), MAX_SPEED) * 10);
    if (lv_arc_get_value(arcMain) != arcVal) {
        lv_arc_set_value(arcGlowWide, arcVal);
        lv_arc_set_value(arcGlow, arcVal);
        lv_arc_set_value(arcMain, arcVal);
        lv_arc_set_value(arcCore, arcVal);
    }

    // SOC + cột pin 5 vạch (mỗi vạch 20%)
    setNum(socLbl, "%d%%", s.soc, true);
    int level = isnan(s.soc) ? 0 : (int)ceilf(s.soc / 20.0f);
    if (level < 0) level = 0;
    if (level > 5) level = 5;
    if (level != lastBattLevel) {
        lastBattLevel = level;
        for (int i = 0; i < 5; i++) {
            lv_obj_set_style_bg_opa(battSeg[i], i < level ? LV_OPA_COVER : 40, 0);
        }
        lv_color_t c = lv_color_hex(SEG_COLORS[level > 0 ? level - 1 : 0]);
        lv_obj_set_style_border_color(battBox, c, 0);
        lv_obj_set_style_outline_color(battBox, c, 0);
        lv_obj_set_style_bg_color(battNub, c, 0);
    }

    // ODO kiểu Việt Nam: 12.450
    if (isnan(s.odo)) {
        setText(odoLbl, "--");
    } else {
        long odo = (long)s.odo;
        if (odo >= 1000) {
            snprintf(buf, sizeof(buf), "%ld.%03ld", odo / 1000, odo % 1000);
        } else {
            snprintf(buf, sizeof(buf), "%ld", odo);
        }
        setText(odoLbl, buf);
    }
    setNum(tripLbl, "%.1f", s.trip);
    setNum(tempLbl, "%d", s.batteryTemp, true);

    setNum(rangeLbl, "%d", s.range, true);
    setNum(voltLbl, "%.2f", s.voltage);
    setNum(currLbl, "%.2f", s.current);

    // chế độ lái
    const char *modeText = x.modeText ? x.modeText : modeTitle(s.mode);
    const uint32_t modeColor = x.modeText ? x.modeColor : modeColorHex(s.mode);
    if (strcmp(modeText, lastModeText) != 0 || modeColor != lastModeColor) {
        snprintf(lastModeText, sizeof(lastModeText), "%s", modeText);
        lastModeColor = modeColor;
        lv_color_t c = lv_color_hex(modeColor);
        setText(modeLbl, modeText);
        lv_obj_set_style_text_color(modeLbl, c, 0);
        lv_obj_set_style_border_color(modeBtn, c, 0);
        lv_obj_set_style_outline_color(modeBtn, c, 0);
        lv_obj_set_style_bg_color(modeBtn, c, 0);
    }

    // trạng thái BMS: chấm xanh khi có dữ liệu thật, xám khi chưa
    if ((int)bmsConnected != lastBms) {
        lastBms = bmsConnected;
        lv_color_t c = bmsConnected ? cGreen() : lv_color_hex(0x5A6478);
        lv_obj_set_style_bg_color(bmsDot, c, 0);
        lv_obj_set_style_outline_color(bmsDot, c, 0);
    }

    // ngày + giờ
    const uint32_t total = buildSeconds + millis() / 1000;
    const uint32_t sec = total % 86400;
    snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)(sec / 3600), (unsigned long)((sec / 60) % 60));
    setText(clockLbl, buf);
    int dd, mm, yy;
    dateAfterDays(total / 86400, dd, mm, yy);
    snprintf(buf, sizeof(buf), "%02d/%02d/%04d", dd, mm, yy);
    setText(dateLbl, buf);
}
