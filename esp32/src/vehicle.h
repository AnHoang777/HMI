#pragma once
// Dữ liệu xe + bộ giả lập (cùng mô hình với app iOS)
#include <stdint.h>

enum class DriveMode : uint8_t { Eco, Normal, Sport };

const char *modeTitle(DriveMode m);
uint32_t modeColorHex(DriveMode m);
DriveMode nextMode(DriveMode m);

struct VehicleState {
    float speed = 32;        // km/h
    float soc = 91;          // %
    float voltage = 66.9f;   // V
    float current = 0;       // A (âm = phanh tái sinh)
    float odo = 12450;       // km
    float trip = 36.8f;      // km
    float batteryTemp = 28;  // °C
    float soh = 100;         // %
    float range = 72;        // km
    DriveMode mode = DriveMode::Eco;

    float power() const { return voltage * current / 1000.0f; }   // kW
};

class VehicleSimulator {
public:
    void step(float dt);
    void setMode(DriveMode mode);
    const VehicleState &state() const { return s_; }

private:
    VehicleState s_;
    float elapsed_ = 0;
    float targetSpeed_ = 32;
    float nextChange_ = 6;
};
