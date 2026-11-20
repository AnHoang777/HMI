// Giao diện bảng đồng hồ neon – LVGL 8.3, màn 800x480.
// Phần tĩnh (viền neon, vạch chia, biểu tượng, hình xe) vẽ trong một lớp "decor";
// phần động (số liệu, cung tốc độ, thanh pin) là các widget LVGL nằm phía trên.
#include "ui.h"

#include <lvgl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "fonts/fonts.h"

namespace {

constexpr int W = 800;
constexpr int H = 480;

// Đồng hồ tốc độ
constexpr int GX = 400;
constexpr int GY = 226;
constexpr int G_R = 140;   // bán kính đường giữa cung
constexpr int G_W = 16;    // độ dày cung
constexpr int MAX_SPEED = 80;
constexpr int TICK_R = 116;

// Cột trái
constexpr int LX = 30;
constexpr int BATT_X = 30, BATT_Y = 146, BATT_W = 170, BATT_H = 46;
constexpr int LROW_Y[3] = {214, 256, 298};
constexpr int LICON_X = 44;

// Cột phải
constexpr int RX = 616;
constexpr int RICON_X = 584;
constexpr int RROW_Y[4] = {90, 156, 222, 288};

// Thanh pin 5 vạch: đỏ → cam → vàng → xanh nõn chuối → xanh lá
constexpr uint32_t SEG_COLORS[5] = {0xFF3B3B, 0xFF8A1C, 0xFFD21C, 0xA6F23D, 0x3DF26A};

const char *RANGE_TEXT = "Ước tính còn đi được";

inline lv_color_t cCyan() { return lv_color_hex(0x19E0FF); }
inline lv_color_t cBlue() { return lv_color_hex(0x1E6BFF); }
inline lv_color_t cGreen() { return lv_color_hex(0x3DF26A); }
inline lv_color_t cAmber() { return lv_color_hex(0xFFB020); }
inline lv_color_t cWhite() { return lv_color_white(); }

// Màu nền (gradient dọc) tại toạ độ y – dùng để trộn màu "phát sáng" mà không cần độ trong suốt
lv_color_t bgAt(int y) {
    if (y < 0) y = 0;
    if (y > H) y = H;
    return lv_color_mix(lv_color_hex(0x040912), lv_color_hex(0x0B1830), (uint8_t)(y * 255 / H));
}

// ---------------------------------------------------------------- hình học tĩnh
lv_point_t topPts[24];
int topN = 0;
lv_point_t sideL[2] = {{14, 356}, {236, 356}};
lv_point_t sideR[2] = {{564, 356}, {786, 356}};
lv_point_t trapPts[8] = {{228, 492}, {292, 379}, {300, 372}, {310, 370},
                         {490, 370}, {500, 372}, {508, 379}, {572, 492}};

int logoX = 0;          // vị trí logo chữ V (tính theo độ rộng chữ)
int pinCx = 0;          // biểu tượng ghim cạnh dòng "Ước tính còn đi được"

void addPt(lv_point_t *a, int &n, float x, float y) {
    a[n].x = (lv_coord_t)lroundf(x);
    a[n].y = (lv_coord_t)lroundf(y);
    n++;
}

void addCubic(lv_point_t *a, int &n, float x0, float y0, float x1, float y1,
              float x2, float y2, float x3, float y3, int steps) {
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / steps, u = 1 - t;
        float x = u * u * u * x0 + 3 * u * u * t * x1 + 3 * u * t * t * x2 + t * t * t * x3;
        float y = u * u * u * y0 + 3 * u * u * t * y1 + 3 * u * t * t * y2 + t * t * t * y3;
        addPt(a, n, x, y);
    }
}

void buildGeometry() {
    topN = 0;
    addPt(topPts, topN, 14, 23);
    addPt(topPts, topN, 210, 23);
    addCubic(topPts, topN, 210, 23, 240, 23, 232, 60, 262, 60, 8);
    addPt(topPts, topN, 538, 60);
    addCubic(topPts, topN, 538, 60, 568, 60, 560, 23, 590, 23, 8);
    addPt(topPts, topN, 786, 23);
}

// ---------------------------------------------------------------- vẽ tĩnh
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

// Đường neon 3 lớp: quầng rộng mờ → quầng vừa → lõi sáng. Màu xanh lam ở hai đầu, cyan ở giữa.
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

void drawRing(lv_draw_ctx_t *ctx, int cx, int cy, int r, int w, lv_color_t c) {
    lv_draw_arc_dsc_t d;
    lv_draw_arc_dsc_init(&d);
    d.color = c;
    d.width = w;
    lv_point_t center = {(lv_coord_t)cx, (lv_coord_t)cy};
    lv_draw_arc(ctx, &d, &center, r, 0, 360);
}

void drawDivider(lv_draw_ctx_t *ctx, int x1, int x2, int y) {
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_opa = LV_OPA_COVER;
    d.bg_color = lv_color_mix(cBlue(), bgAt(y), 170);
    d.bg_grad.dir = LV_GRAD_DIR_HOR;
    d.bg_grad.stops_count = 2;
    d.bg_grad.stops[0].color = d.bg_color;
    d.bg_grad.stops[0].frac = 0;
    d.bg_grad.stops[1].color = bgAt(y);
    d.bg_grad.stops[1].frac = 255;
    lv_area_t a = {(lv_coord_t)x1, (lv_coord_t)y, (lv_coord_t)x2, (lv_coord_t)(y + 1)};
    lv_draw_rect(ctx, &d, &a);
}

// --- biểu tượng
void iconBolt(lv_draw_ctx_t *ctx, int cx, int cy, float s, lv_color_t c) {
    lv_point_t p[4] = {{(lv_coord_t)(cx + 3 * s), (lv_coord_t)(cy - 10 * s)},
                       {(lv_coord_t)(cx - 5 * s), (lv_coord_t)(cy + 1 * s)},
                       {(lv_coord_t)(cx + 5 * s), (lv_coord_t)(cy - 1 * s)},
                       {(lv_coord_t)(cx - 3 * s), (lv_coord_t)(cy + 10 * s)}};
    drawPolyline(ctx, c, (int)(3 * s + 0.5f), p, 4);
}

void iconEcg(lv_draw_ctx_t *ctx, int cx, int cy, float s, lv_color_t c, int w) {
    const float pts[6][2] = {{-11, 0}, {-6, 0}, {-3, -8}, {1, 8}, {4, 0}, {11, 0}};
    lv_point_t p[6];
    for (int i = 0; i < 6; i++) {
        p[i].x = (lv_coord_t)(cx + pts[i][0] * s);
        p[i].y = (lv_coord_t)(cy + pts[i][1] * s);
    }
    drawPolyline(ctx, c, w, p, 6);
}

void iconRoad(lv_draw_ctx_t *ctx, int cx, int cy, lv_color_t c) {
    drawLine(ctx, c, 2, cx - 4, cy - 15, cx - 12, cy + 15);
    drawLine(ctx, c, 2, cx + 4, cy - 15, cx + 12, cy + 15);
    drawLine(ctx, c, 2, cx, cy - 13, cx, cy - 8);
    drawLine(ctx, c, 2, cx, cy - 3, cx, cy + 2);
    drawLine(ctx, c, 2, cx, cy + 7, cx, cy + 13);
}

void iconThermo(lv_draw_ctx_t *ctx, int cx, int cy, lv_color_t c) {
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_opa = LV_OPA_TRANSP;
    d.border_color = c;
    d.border_width = 2;
    d.radius = 5;
    lv_area_t tube = {(lv_coord_t)(cx - 5), (lv_coord_t)(cy - 17), (lv_coord_t)(cx + 5), (lv_coord_t)(cy + 6)};
    lv_draw_rect(ctx, &d, &tube);
    drawCircle(ctx, cx, cy + 10, 7, c, LV_OPA_COVER);
    drawLine(ctx, c, 3, cx, cy - 9, cx, cy + 6);
}

void iconPin(lv_draw_ctx_t *ctx, int cx, int cy, lv_color_t c) {
    drawRing(ctx, cx, cy, 10, 2, c);
    drawCircle(ctx, cx, cy - 1, 4, c, LV_OPA_COVER);
}

void drawChevron(lv_draw_ctx_t *ctx, int x, int y, int w, int h) {
    lv_color_t c = lv_color_hex(0xE8EEF6);
    lv_point_t outer[3] = {{(lv_coord_t)x, (lv_coord_t)y},
                           {(lv_coord_t)(x + w / 2), (lv_coord_t)(y + h)},
                           {(lv_coord_t)(x + w), (lv_coord_t)y}};
    lv_point_t inner[3] = {{(lv_coord_t)(x + w * 0.22f), (lv_coord_t)y},
                           {(lv_coord_t)(x + w / 2), (lv_coord_t)(y + h * 0.62f)},
                           {(lv_coord_t)(x + w * 0.78f), (lv_coord_t)y}};
    drawPolyline(ctx, c, 3, outer, 3);
    drawPolyline(ctx, c, 3, inner, 3);
}

// Hình xe máy điện cách điệu
void drawScooter(lv_draw_ctx_t *ctx) {
    const float s = 0.85f;
    const int ox = 46, oy = 382;
    auto X = [&](float v) { return (int)(ox + v * s); };
    auto Y = [&](float v) { return (int)(oy + v * s); };

    lv_color_t blue = lv_color_hex(0x3D8BFF);
    lv_color_t light = lv_color_hex(0xDDE8FF);

    // quầng sáng dưới xe
    drawCircle(ctx, X(75), Y(70), 62, cBlue(), 28);

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

void drawDecor(lv_event_t *e) {
    lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);

    // quầng sáng sau đồng hồ
    drawCircle(ctx, GX, GY, 168, cBlue(), 20);
    drawCircle(ctx, GX, GY, 128, cBlue(), 20);
    drawCircle(ctx, GX, GY, 88, cBlue(), 18);

    // khung "Ước tính còn đi được"
    lv_draw_rect_dsc_t poly;
    lv_draw_rect_dsc_init(&poly);
    poly.bg_opa = LV_OPA_COVER;
    poly.bg_color = lv_color_mix(cBlue(), bgAt(370), 75);
    poly.bg_grad.dir = LV_GRAD_DIR_VER;
    poly.bg_grad.stops_count = 2;
    poly.bg_grad.stops[0].color = poly.bg_color;
    poly.bg_grad.stops[0].frac = 0;
    poly.bg_grad.stops[1].color = bgAt(H);
    poly.bg_grad.stops[1].frac = 255;
    lv_draw_polygon(ctx, &poly, trapPts, 8);

    // viền neon
    neonPolyline(ctx, topPts, topN, 3, true);
    neonPolyline(ctx, sideL, 2, 2, true);
    neonPolyline(ctx, sideR, 2, 2, true);
    neonPolyline(ctx, trapPts, 8, 3, false);

    // vòng ngoài + rãnh của đồng hồ
    lv_point_t g = {GX, GY};
    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.rounded = 1;
    arc.width = 2;
    arc.color = lv_color_mix(cBlue(), bgAt(GY), 170);
    lv_draw_arc(ctx, &arc, &g, G_R + G_W / 2 + 12, 150, 30);
    arc.width = G_W;
    arc.color = lv_color_mix(cWhite(), bgAt(GY), 22);
    lv_draw_arc(ctx, &arc, &g, G_R + G_W / 2, 150, 30);

    // vạch chia
    for (int i = 0; i <= 16; i++) {
        bool major = (i % 4) == 0;
        float ang = (150.0f + 240.0f * i / 16) * (float)M_PI / 180.0f;
        int len = major ? 13 : 7;
        float r0 = TICK_R - len / 2.0f, r1 = TICK_R + len / 2.0f;
        int x0 = GX + (int)lroundf(r0 * cosf(ang)), y0 = GY + (int)lroundf(r0 * sinf(ang));
        int x1 = GX + (int)lroundf(r1 * cosf(ang)), y1 = GY + (int)lroundf(r1 * sinf(ang));
        drawLine(ctx, lv_color_mix(cWhite(), bgAt(y0), major ? 160 : 70), major ? 3 : 2, x0, y0, x1, y1);
    }

    // logo
    drawChevron(ctx, logoX, 28, 32, 24);

    // cột trái
    lv_color_t iconLeft = lv_color_mix(cCyan(), cBlue(), 120);
    iconBolt(ctx, LICON_X, LROW_Y[0], 1.1f, iconLeft);
    iconEcg(ctx, LICON_X, LROW_Y[1], 1.0f, iconLeft, 2);
    drawRing(ctx, LICON_X, LROW_Y[2], 12, 2, iconLeft);
    iconBolt(ctx, LICON_X, LROW_Y[2], 0.6f, iconLeft);
    drawDivider(ctx, LX, 246, (LROW_Y[0] + LROW_Y[1]) / 2);
    drawDivider(ctx, LX, 246, (LROW_Y[1] + LROW_Y[2]) / 2);

    // cột phải
    iconRoad(ctx, RICON_X, RROW_Y[0], cCyan());
    iconRoad(ctx, RICON_X, RROW_Y[1], cCyan());
    iconThermo(ctx, RICON_X, RROW_Y[2], cCyan());
    iconEcg(ctx, RICON_X, RROW_Y[3], 1.3f, cCyan(), 2);
    for (int i = 0; i < 3; i++) drawDivider(ctx, 556, 784, (RROW_Y[i] + RROW_Y[i + 1]) / 2);

    // dưới
    iconPin(ctx, pinCx, 394, cCyan());
    drawScooter(ctx);
}

// ---------------------------------------------------------------- widget động
ModeTapHandler modeHandler = nullptr;

lv_obj_t *speedLbl, *arcMain, *arcGlow;
lv_obj_t *socLbl, *battBox, *battNub, *battSeg[5];
lv_obj_t *voltLbl, *currLbl, *powerLbl;
lv_obj_t *odoLbl, *tripLbl, *tempLbl, *sohLbl;
lv_obj_t *rangeLbl;
lv_obj_t *modeBtn, *modeLbl;
lv_obj_t *statusDot, *statusLbl;

int lastBattLevel = -1;
int lastMode = -1;
int lastLive = -1;

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

// Hàng "giá trị + đơn vị" căn theo đáy
lv_obj_t *makeValueRow(lv_obj_t *parent, const lv_font_t *fv, const lv_font_t *fu, lv_color_t c,
                       const char *unit, int unitLift, lv_obj_t **valueOut) {
    lv_obj_t *row = makeBox(parent);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(row, 7, 0);
    *valueOut = makeLabel(row, fv, c, "--");
    lv_obj_t *u = makeLabel(row, fu, c, unit);
    lv_obj_set_style_pad_bottom(u, unitLift, 0);
    return row;
}

lv_obj_t *makeArc(lv_obj_t *parent, int width, lv_color_t color, lv_opa_t opa) {
    lv_obj_t *a = lv_arc_create(parent);
    int outer = G_R + width / 2;
    lv_obj_set_size(a, outer * 2, outer * 2);
    placeCenter(a, GX, GY);
    lv_arc_set_rotation(a, 150);
    lv_arc_set_bg_angles(a, 0, 240);
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

}  // namespace

void uiCreate(ModeTapHandler onModeTap) {
    modeHandler = onModeTap;
    buildGeometry();

    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B1830), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x040912), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // --- vị trí logo & ghim (phụ thuộc độ rộng chữ)
    lv_point_t sz;
    lv_txt_get_size(&sz, "VINFAST", &font_mid_24, 8, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    int logoTotal = 32 + 14 + sz.x;
    logoX = W / 2 - logoTotal / 2;

    lv_txt_get_size(&sz, RANGE_TEXT, &font_label_18, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    int rangeTotal = 20 + 8 + sz.x;
    pinCx = W / 2 - rangeTotal / 2 + 10;
    int rangeTextCx = pinCx + 10 + 8 + sz.x / 2;

    // --- lớp vẽ tĩnh
    lv_obj_t *decor = makeBox(scr);
    lv_obj_set_size(decor, W, H);
    lv_obj_set_pos(decor, 0, 0);
    lv_obj_add_event_cb(decor, drawDecor, LV_EVENT_DRAW_MAIN, NULL);

    // --- logo
    lv_obj_t *brand = makeLabel(scr, &font_mid_24, cWhite(), "VINFAST");
    lv_obj_set_style_text_letter_space(brand, 8, 0);
    placeLeft(brand, logoX + 32 + 14, 40);

    // --- đồng hồ tốc độ
    arcGlow = makeArc(scr, 30, cBlue(), 110);
    arcMain = makeArc(scr, G_W, cCyan(), LV_OPA_COVER);

    speedLbl = makeLabel(scr, &font_speed_120, cWhite(), "0");
    placeCenter(speedLbl, GX, 210);

    lv_obj_t *kmh = makeLabel(scr, &font_mid_24, cWhite(), "km/h");
    placeCenter(kmh, GX, 284);

    modeBtn = lv_obj_create(scr);
    lv_obj_remove_style_all(modeBtn);
    lv_obj_clear_flag(modeBtn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(modeBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(modeBtn, 128, 40);
    lv_obj_set_style_radius(modeBtn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(modeBtn, 2, 0);
    lv_obj_set_style_bg_opa(modeBtn, 40, 0);
    lv_obj_set_ext_click_area(modeBtn, 20);
    lv_obj_add_event_cb(modeBtn, onModeClicked, LV_EVENT_CLICKED, NULL);
    modeLbl = makeLabel(modeBtn, &font_mid_24, cGreen(), "ECO");
    lv_obj_center(modeLbl);
    placeCenter(modeBtn, GX, 326);

    // --- quãng đường còn lại
    lv_obj_t *rangeTitle = makeLabel(scr, &font_label_18, lv_color_hex(0xDCE6F5), RANGE_TEXT);
    placeCenter(rangeTitle, rangeTextCx, 394);
    lv_obj_t *rangeRow = makeValueRow(scr, &font_range_46, &font_mid_24, cGreen(), "km", 7, &rangeLbl);
    placeCenter(rangeRow, GX, 438);

    // --- cột trái: SOC, pin, V/A/kW
    lv_obj_t *socTitle = makeLabel(scr, &font_mid_24, cCyan(), "SOC");
    placeLeft(socTitle, LX, 70);
    socLbl = makeLabel(scr, &font_soc_54, cWhite(), "--%");
    placeLeft(socLbl, LX - 2, 114);

    battBox = makeBox(scr);
    lv_obj_set_pos(battBox, BATT_X, BATT_Y);
    lv_obj_set_size(battBox, BATT_W, BATT_H);
    lv_obj_set_style_border_width(battBox, 3, 0);
    lv_obj_set_style_radius(battBox, 10, 0);
    lv_obj_set_style_pad_all(battBox, 5, 0);
    lv_obj_set_style_pad_column(battBox, 5, 0);
    lv_obj_set_flex_flow(battBox, LV_FLEX_FLOW_ROW);
    for (int i = 0; i < 5; i++) {
        battSeg[i] = makeBox(battBox);
        lv_obj_set_height(battSeg[i], lv_pct(100));
        lv_obj_set_flex_grow(battSeg[i], 1);
        lv_obj_set_style_radius(battSeg[i], 3, 0);
        lv_obj_set_style_bg_color(battSeg[i], lv_color_hex(SEG_COLORS[i]), 0);
        lv_obj_set_style_bg_opa(battSeg[i], LV_OPA_COVER, 0);
    }
    battNub = makeBox(scr);
    lv_obj_set_pos(battNub, BATT_X + BATT_W + 3, BATT_Y + (BATT_H - 18) / 2);
    lv_obj_set_size(battNub, 7, 18);
    lv_obj_set_style_radius(battNub, 2, 0);
    lv_obj_set_style_bg_opa(battNub, LV_OPA_COVER, 0);

    lv_obj_t *r;
    r = makeValueRow(scr, &font_value_32, &font_label_18, cWhite(), "V", 5, &voltLbl);
    placeLeft(r, 72, LROW_Y[0]);
    r = makeValueRow(scr, &font_value_32, &font_label_18, cWhite(), "A", 5, &currLbl);
    placeLeft(r, 72, LROW_Y[1]);
    r = makeValueRow(scr, &font_value_32, &font_label_18, cWhite(), "kW", 5, &powerLbl);
    placeLeft(r, 72, LROW_Y[2]);

    // --- cột phải: ODO, TRIP, nhiệt độ pin, SOH
    const char *titles[4] = {"ODO", "TRIP", "Nhiệt độ pin", "SOH"};
    const char *units[4] = {"km", "km", "°C", "%"};
    lv_obj_t **values[4] = {&odoLbl, &tripLbl, &tempLbl, &sohLbl};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *t = makeLabel(scr, &font_label_18, cCyan(), titles[i]);
        placeLeft(t, RX, RROW_Y[i] - 17);
        r = makeValueRow(scr, &font_value_32, &font_label_18, cWhite(), units[i], 5, values[i]);
        placeLeft(r, RX, RROW_Y[i] + 11);
    }

    // --- góc dưới phải: trạng thái nguồn dữ liệu
    lv_obj_t *status = makeBox(scr);
    lv_obj_set_size(status, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(status, 14, 0);
    lv_obj_set_style_pad_ver(status, 7, 0);
    lv_obj_set_style_pad_column(status, 9, 0);
    lv_obj_set_style_radius(status, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(status, 1, 0);
    lv_obj_set_style_border_color(status, lv_color_hex(0x3A4A66), 0);
    statusDot = makeBox(status);
    lv_obj_set_size(statusDot, 10, 10);
    lv_obj_set_style_radius(statusDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(statusDot, LV_OPA_COVER, 0);
    statusLbl = makeLabel(status, &font_label_18, lv_color_hex(0xAEB8C8), "");
    placeCenter(status, 676, 436);
}

void uiUpdate(const VehicleState &s, const char *statusText, bool liveData) {
    char buf[32];

    // tốc độ
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(s.speed));
    setText(speedLbl, buf);
    int arcVal = (int)lroundf(fminf(fmaxf(s.speed, 0), MAX_SPEED) * 10);
    if (lv_arc_get_value(arcMain) != arcVal) {
        lv_arc_set_value(arcMain, arcVal);
        lv_arc_set_value(arcGlow, arcVal);
    }

    // SOC + thanh pin 5 vạch
    int soc = (int)lroundf(s.soc);
    snprintf(buf, sizeof(buf), "%d%%", soc);
    setText(socLbl, buf);
    int level = (int)ceilf(s.soc / 20.0f);
    if (level < 0) level = 0;
    if (level > 5) level = 5;
    if (level != lastBattLevel) {
        lastBattLevel = level;
        for (int i = 0; i < 5; i++) {
            lv_obj_set_style_bg_opa(battSeg[i], i < level ? LV_OPA_COVER : 45, 0);
        }
        lv_color_t c = lv_color_hex(SEG_COLORS[level > 0 ? level - 1 : 0]);
        lv_obj_set_style_border_color(battBox, c, 0);
        lv_obj_set_style_bg_color(battNub, c, 0);
    }

    snprintf(buf, sizeof(buf), "%.1f", s.voltage);
    setText(voltLbl, buf);
    snprintf(buf, sizeof(buf), "%.1f", s.current);
    setText(currLbl, buf);
    snprintf(buf, sizeof(buf), "%.1f", s.power());
    setText(powerLbl, buf);

    // ODO có dấu phẩy ngăn cách hàng nghìn
    long odo = (long)s.odo;
    if (odo >= 1000) {
        snprintf(buf, sizeof(buf), "%ld,%03ld", odo / 1000, odo % 1000);
    } else {
        snprintf(buf, sizeof(buf), "%ld", odo);
    }
    setText(odoLbl, buf);
    snprintf(buf, sizeof(buf), "%.1f", s.trip);
    setText(tripLbl, buf);
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(s.batteryTemp));
    setText(tempLbl, buf);
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(s.soh));
    setText(sohLbl, buf);
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(s.range));
    setText(rangeLbl, buf);

    // chế độ lái
    if ((int)s.mode != lastMode) {
        lastMode = (int)s.mode;
        lv_color_t c = lv_color_hex(modeColorHex(s.mode));
        setText(modeLbl, modeTitle(s.mode));
        lv_obj_set_style_text_color(modeLbl, c, 0);
        lv_obj_set_style_border_color(modeBtn, c, 0);
        lv_obj_set_style_bg_color(modeBtn, c, 0);
    }

    // trạng thái dữ liệu
    setText(statusLbl, statusText);
    if ((int)liveData != lastLive) {
        lastLive = liveData;
        lv_obj_set_style_bg_color(statusDot, liveData ? cGreen() : cAmber(), 0);
    }
}
