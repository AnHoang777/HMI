#include "vehicle.h"

#include <Arduino.h>
#include <math.h>

namespace {

// Pack pin giả lập: 16S Li-ion (~60 V danh định), 40 Ah ≈ 2.4 kWh
constexpr float CELLS = 16;
constexpr float CAPACITY_AH = 40;
constexpr float NOMINAL_V = 59.2f;
constexpr float R_INTERNAL = 0.06f;

float frand() { return (float)esp_random() / 4294967296.0f; }
float frandRange(float a, float b) { return a + (b - a) * frand(); }
float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

float maxSpeed(DriveMode m) {
    switch (m) {
        case DriveMode::Normal: return 55;
        case DriveMode::Sport: return 70;
        default: return 45;
    }
}

float maxAccel(DriveMode m) {
    switch (m) {
        case DriveMode::Normal: return 8;
        case DriveMode::Sport: return 12;
        default: return 5;
    }
}

float consumptionWhPerKm(DriveMode m) {
    switch (m) {
        case DriveMode::Normal: return 34;
        case DriveMode::Sport: return 40;
        default: return 30;
    }
}

}  // namespace

const char *modeTitle(DriveMode m) {
    switch (m) {
        case DriveMode::Normal: return "NORMAL";
        case DriveMode::Sport: return "SPORT";
        default: return "ECO";
    }
}

uint32_t modeColorHex(DriveMode m) {
    switch (m) {
        case DriveMode::Normal: return 0x19E0FF;
        case DriveMode::Sport: return 0xFF9F1C;
        default: return 0x3DF26A;
    }
}

DriveMode nextMode(DriveMode m) {
    switch (m) {
        case DriveMode::Eco: return DriveMode::Normal;
        case DriveMode::Normal: return DriveMode::Sport;
        default: return DriveMode::Eco;
    }
}

void VehicleSimulator::setMode(DriveMode mode) {
    s_.mode = mode;
    targetSpeed_ = fminf(targetSpeed_, maxSpeed(mode));
}

void VehicleSimulator::step(float dt) {
    elapsed_ += dt;

    // Tốc độ mục tiêu mới sau mỗi vài giây (20% khả năng dừng đèn đỏ)
    if (elapsed_ >= nextChange_) {
        if (frand() < 0.2f) {
            targetSpeed_ = 0;
            nextChange_ = elapsed_ + frandRange(3, 6);
        } else {
            targetSpeed_ = frandRange(15, maxSpeed(s_.mode));
            nextChange_ = elapsed_ + frandRange(5, 12);
        }
    }

    const float accel = clampf((targetSpeed_ - s_.speed) * 0.8f, -12, maxAccel(s_.mode));
    s_.speed = fmaxf(0, s_.speed + accel * dt);
    const float v = s_.speed;

    // Dòng điện: cản lăn + cản gió + tăng tốc; phanh mạnh → tái sinh (âm)
    float current;
    if (v < 0.5f && accel <= 0) {
        current = 0;
    } else if (accel < -1.5f) {
        current = accel * 1.2f;
    } else {
        current = 0.6f + 0.09f * v + 0.0025f * v * v + fmaxf(accel, 0) * 3 + frandRange(-0.15f, 0.15f);
    }
    s_.current = current;

    s_.soc = clampf(s_.soc - current * dt / 3600 / CAPACITY_AH * 100, 0, 100);

    const float cellOcv = 3.40f + 0.82f * s_.soc / 100;
    s_.voltage = cellOcv * CELLS - current * R_INTERNAL;

    const float km = v * dt / 3600;
    s_.odo += km;
    s_.trip += km;

    const float equilibrium = 27 + current * current * 0.004f;
    s_.batteryTemp += (equilibrium - s_.batteryTemp) * dt / 90;

    const float remainingWh = s_.soc / 100 * CAPACITY_AH * NOMINAL_V * s_.soh / 100;
    s_.range = remainingWh / consumptionWhPerKm(s_.mode);
}
