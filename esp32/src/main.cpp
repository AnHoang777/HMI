// Bảng đồng hồ xe máy điện – Waveshare ESP32-S3-Touch-LCD-4.3
// Hiện tại chạy dữ liệu giả lập; chạm vào nút ECO để đổi chế độ lái.
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <lvgl.h>

#include "board.h"
#include "ui.h"
#include "vehicle.h"

namespace {

constexpr size_t DRAW_LINES = 40;

lv_disp_draw_buf_t drawBuf;
lv_disp_drv_t dispDrv;
lv_indev_drv_t indevDrv;
VehicleSimulator sim;

void flushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *pixels) {
    boardDrawBitmap(area->x1, area->y1, area->x2, area->y2, pixels);
    lv_disp_flush_ready(drv);
}

void touchCb(lv_indev_drv_t *, lv_indev_data_t *data) {
    uint16_t x, y;
    bool pressed = boardReadTouch(x, y);
    data->state = pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    data->point.x = x;
    data->point.y = y;
}

lv_color_t *allocDrawBuffer(size_t pixels) {
    // ưu tiên RAM trong (nhanh), thiếu thì dùng PSRAM
    void *p = heap_caps_malloc(pixels * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!p) p = heap_caps_malloc(pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    return (lv_color_t *)p;
}

void onModeTap() {
    sim.setMode(nextMode(sim.state().mode));
}

}  // namespace

void setup() {
    Serial.begin(115200);
    Serial.println("HMI dashboard – khởi động");

    boardInit();
    lv_init();

    const size_t pixels = LCD_W * DRAW_LINES;
    lv_color_t *buf1 = allocDrawBuffer(pixels);
    lv_color_t *buf2 = allocDrawBuffer(pixels);
    lv_disp_draw_buf_init(&drawBuf, buf1, buf2, pixels);

    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res = LCD_W;
    dispDrv.ver_res = LCD_H;
    dispDrv.flush_cb = flushCb;
    dispDrv.draw_buf = &drawBuf;
    lv_disp_drv_register(&dispDrv);

    lv_indev_drv_init(&indevDrv);
    indevDrv.type = LV_INDEV_TYPE_POINTER;
    indevDrv.read_cb = touchCb;
    lv_indev_drv_register(&indevDrv);

    uiCreate(onModeTap);
    uiUpdate(sim.state(), "Dữ liệu giả lập", false);
}

void loop() {
    static uint32_t lastStep = 0;
    const uint32_t now = millis();

    if (now - lastStep >= 100) {
        lastStep = now;
        sim.step(0.1f);
        uiUpdate(sim.state(), "Dữ liệu giả lập", false);
    }

    lv_timer_handler();
    delay(5);
}
