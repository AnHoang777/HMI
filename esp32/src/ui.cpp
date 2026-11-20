// Giao diện bảng đồng hồ neon – LVGL 8.3, màn 800x480.
//
//  ┌ logo VINFAST ───────────────────────── BT ● Kết nối BMS  14:32 ┐
//  │ [pin dọc] SOC 91% │      ( đồng hồ tròn )      │ ODO           │
//  │ [ hình xe ]       │   39 km/h   [ECO]          │ TRIP          │
//  │                   │                            │ Nhiệt độ pin  │
//  │ ⚡ Ước tính còn đi được 72 km │ V VINFAST │ Điện áp pin / Dòng điện │
//  └──────────────── 🍃 Vì một tương lai xanh 🍃 ─────────────────────┘
//
// Lớp nền (viền neon, vòng đồng hồ, vạch chia) và lớp trước (biểu tượng, logo, hình xe)
// được vẽ trực tiếp; số liệu, cung tốc độ, cột pin là widget LVGL.
#include "ui.h"

#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "fonts/fonts.h"

namespace {

constexpr int W = 800;
constexpr int H = 480;

struct Box {
    int x, y, w, h;
    int cy() const { return y + h / 2; }
};

// Các ô thẻ
constexpr Box CARD_SOC{12, 66, 220, 130};
constexpr Box CARD_BIKE{12, 204, 220, 124};
constexpr Box CARD_R[3] = {{568, 66, 220, 82}, {568, 156, 220, 82}, {568, 246, 220, 82}};
constexpr Box CARD_BOTTOM{12, 338, 776, 108};

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
constexpr int BATT_X = 30, BATT_Y = 88, BATT_W = 46, BATT_H = 98;
constexpr int NUB_W = 22, NUB_H = 7;
constexpr uint32_t SEG_COLORS[5] = {0xFF3B3B, 0xFF8A1C, 0xFFD21C, 0xA6F23D, 0x3DF26A};

// Hàng điện áp / dòng điện trong ô dưới
constexpr int VA_ICON_X = 516;
constexpr int VA_TEXT_X = 542;
constexpr int VA_ROW_Y[2] = {367, 415};

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
lv_point_t headerPts[6] = {{14, 58}, {236, 58}, {256, 44}, {544, 44}, {564, 58}, {786, 58}};
lv_point_t topPts[6] = {{40, 6}, {250, 6}, {266, 18}, {534, 18}, {550, 6}, {760, 6}};
lv_point_t footL[3] = {{14, 448}, {30, 466}, {272, 466}};
lv_point_t footR[3] = {{528, 466}, {770, 466}, {786, 448}};

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

void drawPolyline(lv_draw_ctx_t *ctx, lv_color_t c, int w, const lv_point_t *p, int n) {
    for (int i = 0; i < n - 1; i++) drawLine(ctx, c, w, p[i].x, p[i].y, p[i + 1].x, p[i + 1].y);
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

// Vòng tròn neon (dùng cho khung icon và vòng ngoài đồng hồ)
void neonRing(lv_draw_ctx_t *ctx, int cx, int cy, int r, int w, lv_color_t c, lv_color_t bg) {
    drawRing(ctx, cx, cy, r + 4, w + 8, lv_color_mix(c, bg, 40));
    drawRing(ctx, cx, cy, r + 2, w + 4, lv_color_mix(c, bg, 100));
    drawRing(ctx, cx, cy, r, w, lv_color_mix(cWhite(), c, 50));
}

// ---------------------------------------------------------------- biểu tượng
void iconBolt(lv_draw_ctx_t *ctx, int cx, int cy, float s, lv_color_t c) {
    lv_point_t p[4] = {{(lv_coord_t)(cx + 3 * s), (lv_coord_t)(cy - 10 * s)},
                       {(lv_coord_t)(cx - 5 * s), (lv_coord_t)(cy + 1 * s)},
                       {(lv_coord_t)(cx + 5 * s), (lv_coord_t)(cy - 1 * s)},
                       {(lv_coord_t)(cx - 3 * s), (lv_coord_t)(cy + 10 * s)}};
    drawPolyline(ctx, c, (int)(3 * s + 0.5f), p, 4);
}

void iconRoad(lv_draw_ctx_t *ctx, int cx, int cy, lv_color_t c) {
    drawLine(ctx, c, 2, cx - 3, cy - 11, cx - 9, cy + 11);
    drawLine(ctx, c, 2, cx + 3, cy - 11, cx + 9, cy + 11);
    drawLine(ctx, c, 2, cx, cy - 9, cx, cy - 5);
    drawLine(ctx, c, 2, cx, cy - 1, cx, cy + 3);
    drawLine(ctx, c, 2, cx, cy + 7, cx, cy + 10);
}

void iconThermo(lv_draw_ctx_t *ctx, int cx, int cy, lv_color_t c) {
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_opa = LV_OPA_TRANSP;
    d.border_color = c;
    d.border_width = 2;
    d.radius = 4;
    lv_area_t tube = {(lv_coord_t)(cx - 4), (lv_coord_t)(cy - 13), (lv_coord_t)(cx + 4), (lv_coord_t)(cy + 4)};
    lv_draw_rect(ctx, &d, &tube);
    drawCircle(ctx, cx, cy + 8, 6, c, LV_OPA_COVER);
    drawLine(ctx, c, 2, cx, cy - 7, cx, cy + 4);
}

void iconPlug(lv_draw_ctx_t *ctx, int cx, int cy, lv_color_t c) {
    drawLine(ctx, c, 2, cx - 4, cy - 12, cx - 4, cy - 6);   // chân cắm
    drawLine(ctx, c, 2, cx + 4, cy - 12, cx + 4, cy - 6);
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_opa = LV_OPA_TRANSP;
    d.border_color = c;
    d.border_width = 2;
    d.radius = 4;
    lv_area_t body = {(lv_coord_t)(cx - 8), (lv_coord_t)(cy - 6), (lv_coord_t)(cx + 8), (lv_coord_t)(cy + 4)};
    lv_draw_rect(ctx, &d, &body);
    drawLine(ctx, c, 2, cx, cy + 4, cx, cy + 12);           // dây
}

void iconLeaf(lv_draw_ctx_t *ctx, int cx, int cy, bool mirror, lv_color_t c) {
    int s = mirror ? -1 : 1;
    drawLine(ctx, c, 9, cx - 4 * s, cy + 4, cx + 4 * s, cy - 4);
    drawLine(ctx, lv_color_hex(0x07142A), 1, cx - 5 * s, cy + 5, cx + 3 * s, cy - 3);
}

void drawChevron(lv_draw_ctx_t *ctx, int x, int y, int w, int h, int stroke) {
    lv_color_t c = lv_color_hex(0xE8EEF6);
    lv_point_t outer[3] = {{(lv_coord_t)x, (lv_coord_t)y},
                           {(lv_coord_t)(x + w / 2), (lv_coord_t)(y + h)},
                           {(lv_coord_t)(x + w), (lv_coord_t)y}};
    lv_point_t inner[3] = {{(lv_coord_t)(x + w * 0.22f), (lv_coord_t)y},
                           {(lv_coord_t)(x + w / 2), (lv_coord_t)(y + h * 0.62f)},
                           {(lv_coord_t)(x + w * 0.78f), (lv_coord_t)y}};
    drawPolyline(ctx, c, stroke, outer, 3);
    drawPolyline(ctx, c, stroke, inner, 3);
}

// Hình xe máy điện cách điệu, căn giữa tại (cx, cy)
void drawScooter(lv_draw_ctx_t *ctx, int cx, int cy) {
    const float s = 0.9f;
    const int ox = cx - (int)(69 * s), oy = cy - (int)(56 * s);
    auto X = [&](float v) { return (int)(ox + v * s); };
    auto Y = [&](float v) { return (int)(oy + v * s); };

    lv_color_t blue = lv_color_hex(0x3D8BFF);
    lv_color_t light = lv_color_hex(0xDDE8FF);

    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.radius = (lv_coord_t)(12 * s);
    d.bg_opa = LV_OPA_COVER;
    d.bg_color = light;
    d.bg_grad.dir = LV_GRAD_DIR_VER;
    d.bg_grad.stops_count = 2;
    d.bg_grad.stops[0].color = light;
    d.bg_grad.stops[0].frac = 0;
    d.bg_grad.stops[1].color = blue;
    d.bg_grad.stops[1].frac = 255;
    lv_area_t body = {(lv_coord_t)X(14), (lv_coord_t)Y(46), (lv_coord_t)X(96), (lv_coord_t)Y(70)};
    lv_draw_rect(ctx, &d, &body);

    lv_draw_rect_dsc_t seat;
    lv_draw_rect_dsc_init(&seat);
    seat.radius = 5;
    seat.bg_opa = LV_OPA_COVER;
    seat.bg_color = light;
    lv_area_t seatA = {(lv_coord_t)X(44), (lv_coord_t)Y(34), (lv_coord_t)X(80), (lv_coord_t)Y(44)};
    lv_draw_rect(ctx, &seat, &seatA);

    drawLine(ctx, blue, 6, X(94), Y(68), X(112), Y(22));     // cổ lái
    drawLine(ctx, light, 6, X(104), Y(20), X(124), Y(17));   // tay lái
    drawLine(ctx, blue, 5, X(112), Y(24), X(122), Y(64));    // phuộc trước

    drawRing(ctx, X(28), Y(78), (int)(17 * s), 6, blue);
    drawRing(ctx, X(122), Y(78), (int)(17 * s), 6, blue);
    drawCircle(ctx, X(28), Y(78), 4, light, LV_OPA_COVER);
    drawCircle(ctx, X(122), Y(78), 4, light, LV_OPA_COVER);
}

// ---------------------------------------------------------------- lớp vẽ
// Lớp nền: nằm dưới các ô thẻ
void drawBack(lv_event_t *e) {
    lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);

    // viền neon trên/dưới
    neonPolyline(ctx, topPts, 6, 2, true);
    neonPolyline(ctx, headerPts, 6, 3, true);
    neonPolyline(ctx, footL, 3, 2, false);
    neonPolyline(ctx, footR, 3, 2, false);

    // đồng hồ: quầng sáng, lòng tối, vòng neon ngoài, rãnh cung, vạch chia
    drawCircle(ctx, GX, GY, G_RING + 22, cBlue(), 22);
    drawCircle(ctx, GX, GY, G_RING - 4, lv_color_hex(0x050C1A), 210);
    neonRing(ctx, GX, GY, G_RING, 2, cBlue(), bgAt(GY));

    drawArc(ctx, GX, GY, G_R + G_W / 2, G_W, lv_color_mix(cWhite(), bgAt(GY), 22),
            G_START, (G_START + G_SWEEP) % 360, true);

    for (int i = 0; i <= 20; i++) {
        bool major = (i % 5) == 0;
        float ang = (G_START + (float)G_SWEEP * i / 20) * (float)M_PI / 180.0f;
        int len = major ? 12 : 6;
        float r0 = TICK_R - len / 2.0f, r1 = TICK_R + len / 2.0f;
        int x0 = GX + (int)lroundf(r0 * cosf(ang)), y0 = GY + (int)lroundf(r0 * sinf(ang));
        int x1 = GX + (int)lroundf(r1 * cosf(ang)), y1 = GY + (int)lroundf(r1 * sinf(ang));
        drawLine(ctx, lv_color_mix(cWhite(), bgAt(y0), major ? 170 : 70), major ? 3 : 2, x0, y0, x1, y1);
    }
}

// Lớp trước: nằm trên các ô thẻ, dưới chữ số
void drawFront(lv_event_t *e) {
    lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
    lv_color_t cardBg = lv_color_hex(0x0A1830);

    // logo đầu màn
    drawChevron(ctx, 24, 18, 34, 26, 3);

    // ô hình xe
    drawScooter(ctx, CARD_BIKE.x + CARD_BIKE.w / 2, CARD_BIKE.cy());

    // ô bên phải: icon trong vòng tròn
    for (int i = 0; i < 3; i++) neonRing(ctx, 600, CARD_R[i].cy(), 20, 2, cCyan(), cardBg);
    iconRoad(ctx, 600, CARD_R[0].cy(), cCyan());
    iconRoad(ctx, 600, CARD_R[1].cy(), cCyan());
    iconThermo(ctx, 600, CARD_R[2].cy(), cCyan());

    // ô dưới: tia sét, logo giữa, vạch chia, điện áp & dòng điện
    neonRing(ctx, 52, CARD_BOTTOM.cy(), 22, 2, cCyan(), cardBg);
    iconBolt(ctx, 52, CARD_BOTTOM.cy(), 1.2f, cCyan());

    drawChevron(ctx, 383, 352, 44, 34, 3);

    lv_draw_rect_dsc_t div;
    lv_draw_rect_dsc_init(&div);
    div.bg_opa = LV_OPA_COVER;
    div.bg_color = lv_color_hex(0x2A5AA8);
    lv_area_t divA = {488, 356, 489, 428};
    lv_draw_rect(ctx, &div, &divA);

    iconPlug(ctx, VA_ICON_X, VA_ROW_Y[0], cCyan());
    drawRing(ctx, VA_ICON_X, VA_ROW_Y[1], 12, 2, cCyan());
    iconBolt(ctx, VA_ICON_X, VA_ROW_Y[1], 0.65f, cCyan());

    // khẩu hiệu cuối màn
    iconLeaf(ctx, 292, 466, false, cCyan());
    iconLeaf(ctx, 508, 466, true, cCyan());
}

// ---------------------------------------------------------------- widget
ModeTapHandler modeHandler = nullptr;

lv_obj_t *speedLbl, *arcMain, *arcGlow;
lv_obj_t *socLbl, *battBox, *battNub, *battSeg[5];
lv_obj_t *odoLbl, *tripLbl, *tempLbl;
lv_obj_t *rangeLbl, *voltLbl, *currLbl;
lv_obj_t *modeBtn, *modeLbl;
lv_obj_t *bmsDot, *clockLbl;

int lastBattLevel = -1;
int lastMode = -1;
int lastBms = -1;
int buildSeconds = 0;

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

lv_obj_t *makeLayer(lv_obj_t *parent, lv_event_cb_t cb) {
    lv_obj_t *o = makeBox(parent);
    lv_obj_set_size(o, W, H);
    lv_obj_set_pos(o, 0, 0);
    lv_obj_add_event_cb(o, cb, LV_EVENT_DRAW_MAIN, NULL);
    return o;
}

// Ô thẻ bo góc, viền xanh, quầng sáng nhẹ bên ngoài
void makeCard(lv_obj_t *parent, const Box &b) {
    lv_obj_t *c = makeBox(parent);
    lv_obj_set_pos(c, b.x, b.y);
    lv_obj_set_size(c, b.w, b.h);
    lv_obj_set_style_radius(c, 14, 0);
    lv_obj_set_style_bg_color(c, lv_color_hex(0x0A1830), 0);
    lv_obj_set_style_bg_opa(c, 220, 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_border_color(c, lv_color_hex(0x2F74E0), 0);
    lv_obj_set_style_outline_width(c, 3, 0);
    lv_obj_set_style_outline_color(c, cBlue(), 0);
    lv_obj_set_style_outline_opa(c, 45, 0);
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
void makeStat(lv_obj_t *parent, int x, int cy, const char *title, const char *unit, lv_obj_t **valueOut) {
    lv_obj_t *t = makeLabel(parent, &font_label_16, cLabel(), title);
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
    if (modeHandler) modeHandler();
}

// Đồng hồ: chưa có nguồn giờ thật (RTC / WiFi / điện thoại) nên tạm tính từ giờ build firmware (UTC+7)
int parseBuildSeconds() {
    const char *t = __TIME__;   // "HH:MM:SS"
    int h = (t[0] - '0') * 10 + (t[1] - '0');
    int m = (t[3] - '0') * 10 + (t[4] - '0');
    int s = (t[6] - '0') * 10 + (t[7] - '0');
    return ((h + 7) % 24) * 3600 + m * 60 + s;
}

}  // namespace

void uiCreate(ModeTapHandler onModeTap) {
    modeHandler = onModeTap;
    buildSeconds = parseBuildSeconds();

    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A1630), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x040912), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // --- lớp nền + ô thẻ + lớp trước
    makeLayer(scr, drawBack);
    makeCard(scr, CARD_SOC);
    makeCard(scr, CARD_BIKE);
    for (int i = 0; i < 3; i++) makeCard(scr, CARD_R[i]);
    makeCard(scr, CARD_BOTTOM);
    makeLayer(scr, drawFront);

    // --- đầu màn: logo + trạng thái BMS + giờ
    lv_obj_t *brand = makeLabel(scr, &font_mid_22, cWhite(), "VINFAST");
    lv_obj_set_style_text_letter_space(brand, 6, 0);
    placeLeft(brand, 70, 31);

    lv_obj_t *status = makeBox(scr);
    lv_obj_set_size(status, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(status, 8, 0);
    makeLabel(status, &lv_font_montserrat_20, cBlue(), LV_SYMBOL_BLUETOOTH);
    bmsDot = makeBox(status);
    lv_obj_set_size(bmsDot, 10, 10);
    lv_obj_set_style_radius(bmsDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(bmsDot, LV_OPA_COVER, 0);
    makeLabel(status, &font_label_16, lv_color_hex(0xDCE6F5), "Kết nối BMS");
    clockLbl = makeLabel(status, &font_mid_22, cWhite(), "--:--");
    lv_obj_set_style_pad_left(clockLbl, 14, 0);
    lv_obj_align(status, LV_ALIGN_TOP_RIGHT, -22, 17);

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
    placeLeft(socTitle, 98, 106);
    socLbl = makeLabel(scr, &font_soc_44, cWhite(), "--%");
    placeLeft(socLbl, 96, 150);

    // --- đồng hồ tốc độ
    arcGlow = makeArc(scr, 28, cBlue(), 120);
    arcMain = makeArc(scr, G_W, cCyan(), LV_OPA_COVER);

    speedLbl = makeLabel(scr, &font_speed_104, cWhite(), "0");
    placeCenter(speedLbl, GX, 188);

    lv_obj_t *kmh = makeLabel(scr, &font_mid_22, cWhite(), "km/h");
    placeCenter(kmh, GX, 246);

    modeBtn = lv_obj_create(scr);
    lv_obj_remove_style_all(modeBtn);
    lv_obj_clear_flag(modeBtn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(modeBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(modeBtn, 110, 34);
    lv_obj_set_style_radius(modeBtn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(modeBtn, 2, 0);
    lv_obj_set_style_bg_opa(modeBtn, 40, 0);
    lv_obj_set_ext_click_area(modeBtn, 20);
    lv_obj_add_event_cb(modeBtn, onModeClicked, LV_EVENT_CLICKED, NULL);
    modeLbl = makeLabel(modeBtn, &font_mid_22, cGreen(), "ECO");
    lv_obj_center(modeLbl);
    placeCenter(modeBtn, GX, 284);

    // --- ô bên phải
    makeStat(scr, 632, CARD_R[0].cy(), "ODO", "km", &odoLbl);
    makeStat(scr, 632, CARD_R[1].cy(), "TRIP", "km", &tripLbl);
    makeStat(scr, 632, CARD_R[2].cy(), "Nhiệt độ pin", "°C", &tempLbl);

    // --- ô dưới: quãng đường còn lại | logo | điện áp, dòng điện
    lv_obj_t *rangeTitle = makeLabel(scr, &font_label_16, lv_color_hex(0xDCE6F5), "Ước tính còn đi được");
    placeLeft(rangeTitle, 88, 370);
    lv_obj_t *rangeRow = makeValueRow(scr, &font_range_44, &font_mid_22, cGreen(), "km", 5, &rangeLbl);
    placeLeft(rangeRow, 88, 410);

    lv_obj_t *brand2 = makeLabel(scr, &font_label_16, cWhite(), "VINFAST");
    lv_obj_set_style_text_letter_space(brand2, 4, 0);
    placeCenter(brand2, 405, 410);

    makeStat(scr, VA_TEXT_X, VA_ROW_Y[0] - 2, "Điện áp pin", "V", &voltLbl);
    makeStat(scr, VA_TEXT_X, VA_ROW_Y[1] + 2, "Dòng điện", "A", &currLbl);

    // --- khẩu hiệu
    lv_obj_t *slogan = makeLabel(scr, &font_italic_16, lv_color_hex(0x7FB8FF), "Vì một tương lai xanh");
    placeCenter(slogan, GX, 466);
}

void uiUpdate(const VehicleState &s, bool bmsConnected) {
    char buf[32];

    // tốc độ
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(s.speed));
    setText(speedLbl, buf);
    int arcVal = (int)lroundf(fminf(fmaxf(s.speed, 0), MAX_SPEED) * 10);
    if (lv_arc_get_value(arcMain) != arcVal) {
        lv_arc_set_value(arcMain, arcVal);
        lv_arc_set_value(arcGlow, arcVal);
    }

    // SOC + cột pin 5 vạch (mỗi vạch 20%)
    snprintf(buf, sizeof(buf), "%d%%", (int)lroundf(s.soc));
    setText(socLbl, buf);
    int level = (int)ceilf(s.soc / 20.0f);
    if (level < 0) level = 0;
    if (level > 5) level = 5;
    if (level != lastBattLevel) {
        lastBattLevel = level;
        for (int i = 0; i < 5; i++) {
            lv_obj_set_style_bg_opa(battSeg[i], i < level ? LV_OPA_COVER : 40, 0);
        }
        lv_color_t c = lv_color_hex(SEG_COLORS[level > 0 ? level - 1 : 0]);
        lv_obj_set_style_border_color(battBox, c, 0);
        lv_obj_set_style_bg_color(battNub, c, 0);
    }

    // ODO kiểu Việt Nam: 12.450
    long odo = (long)s.odo;
    if (odo >= 1000) {
        snprintf(buf, sizeof(buf), "%ld.%03ld", odo / 1000, odo % 1000);
    } else {
        snprintf(buf, sizeof(buf), "%ld", odo);
    }
    setText(odoLbl, buf);
    snprintf(buf, sizeof(buf), "%.1f", s.trip);
    setText(tripLbl, buf);
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(s.batteryTemp));
    setText(tempLbl, buf);

    snprintf(buf, sizeof(buf), "%d", (int)lroundf(s.range));
    setText(rangeLbl, buf);
    snprintf(buf, sizeof(buf), "%.2f", s.voltage);
    setText(voltLbl, buf);
    snprintf(buf, sizeof(buf), "%.2f", s.current);
    setText(currLbl, buf);

    // chế độ lái
    if ((int)s.mode != lastMode) {
        lastMode = (int)s.mode;
        lv_color_t c = lv_color_hex(modeColorHex(s.mode));
        setText(modeLbl, modeTitle(s.mode));
        lv_obj_set_style_text_color(modeLbl, c, 0);
        lv_obj_set_style_border_color(modeBtn, c, 0);
        lv_obj_set_style_bg_color(modeBtn, c, 0);
    }

    // trạng thái BMS: chấm xanh khi có dữ liệu thật, xám khi chưa
    if ((int)bmsConnected != lastBms) {
        lastBms = bmsConnected;
        lv_obj_set_style_bg_color(bmsDot, bmsConnected ? cGreen() : lv_color_hex(0x5A6478), 0);
    }

    // giờ
    uint32_t sec = (buildSeconds + millis() / 1000) % 86400;
    snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)(sec / 3600), (unsigned long)((sec / 60) % 60));
    setText(clockLbl, buf);
}
