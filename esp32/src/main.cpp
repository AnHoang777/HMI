// Bảng đồng hồ xe máy điện – Waveshare ESP32-S3-Touch-LCD-4.3
// SOC, điện áp, dòng điện, nhiệt độ pin đọc từ BMS qua CAN (bms.cpp).
// Không có dữ liệu thật thì hiện "--". Đặt DEMO_SIMULATION = true để chạy số liệu giả lập.
// Chạm nút ECO để đổi chế độ lái.
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <lvgl.h>
#include <math.h>

#include "bms.h"
#include "board.h"
#include "ui.h"
#include "vehicle.h"

namespace {

constexpr size_t DRAW_LINES = 40;
constexpr bool DEMO_SIMULATION = false;   // true = chạy dữ liệu giả lập để trưng bày

// Trạng thái hiển thị chỉ gồm dữ liệu thật; NAN = chưa có
VehicleState realState() {
    VehicleState s;
    const BmsData &b = bmsData();
    s.speed = NAN;          // chưa có nguồn tốc độ
    s.odo = NAN;
    s.trip = NAN;
    s.range = NAN;
    s.soh = b.sohValid ? b.soh : NAN;
    s.soc = b.socValid ? b.soc : NAN;
    s.voltage = b.packValid ? b.voltage : NAN;
    s.current = b.packValid ? b.current : NAN;
    s.batteryTemp = b.tempValid ? b.batteryTemp() : NAN;
    return s;
}

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

    bmsBegin();

    uiCreate(onModeTap);
    uiUpdate(DEMO_SIMULATION ? sim.state() : realState(), false);
}

void loop() {
    static uint32_t lastStep = 0;
    const uint32_t now = millis();

    bmsPoll();

    if (now - lastStep >= 100) {
        lastStep = now;
        if (DEMO_SIMULATION) {
            sim.step(0.1f);
            uiUpdate(sim.state(), bmsOnline());
        } else {
            VehicleState s = realState();
            s.mode = sim.state().mode;    // chế độ lái do người dùng chọn
            uiUpdate(s, bmsOnline());
        }
    }

    lv_timer_handler();
    delay(5);
}
