#include "vcu.h"

#include <Arduino.h>
#include <Preferences.h>
#include <stdio.h>

namespace {

constexpr uint32_t FRESH_MS = 3000;
constexpr float SPEED_KMH_PER_COUNT = 0.1f;
constexpr float ODO_KM_PER_COUNT = 0.1f;

Preferences prefs;
VcuData v;

uint32_t last102Ms = 0, last201Ms = 0, lastLogMs = 0;
bool haveOdo = false;
bool tripStartSet = false;
uint32_t tripStartRaw = 0;

uint16_t u16be(const uint8_t *d) { return (uint16_t(d[0]) << 8) | d[1]; }
bool fresh(uint32_t t, uint32_t now) { return t != 0 && now - t <= FRESH_MS; }

void setTripStart(uint16_t odo) {
    tripStartRaw = odo;
    tripStartSet = true;
    prefs.putUInt("trip0", tripStartRaw);
}

void onOdo(uint16_t odo) {
    v.odoRaw = odo;
    haveOdo = true;
    // lần đầu chạy, hoặc ODO nhỏ hơn mốc (thay bộ điều khiển / tràn số) → TRIP về 0
    if (!tripStartSet || odo < tripStartRaw) setTripStart(odo);
}

}  // namespace

void vcuBegin() {
    prefs.begin("vcu", false);
    tripStartSet = prefs.isKey("trip0");
    tripStartRaw = prefs.getUInt("trip0", 0);
}

void vcuDecode(uint32_t id, const uint8_t *d, uint8_t len, uint32_t now) {
    if (len < 8) return;
    switch (id) {
        case 0x102:
            v.stateRaw = d[0];
            v.ready = d[0] & 0x10;
            v.sport = d[0] & 0x01;
            v.standDown = d[1] & 0x10;
            v.dist100m = d[7];
            last102Ms = now;
            break;
        case 0x201:
            v.speedRaw = u16be(&d[0]);
            v.brake = d[7] & 0x10;
            onOdo(u16be(&d[4]));
            last201Ms = now;
            break;
    }
}

void vcuUpdate() {
    const uint32_t now = millis();
    v.stateValid = fresh(last102Ms, now);
    v.motionValid = fresh(last201Ms, now);

    v.speedKmh = v.motionValid ? v.speedRaw * SPEED_KMH_PER_COUNT : NAN;
    v.odoKm = (v.motionValid && haveOdo) ? v.odoRaw * ODO_KM_PER_COUNT : NAN;
    v.tripKm = (v.motionValid && haveOdo && tripStartSet) ? (v.odoRaw - tripStartRaw) * ODO_KM_PER_COUNT : NAN;

    if (now - lastLogMs >= 1000) {
        lastLogMs = now;
        Serial.printf("[VCU] D0=%02X %s %s stand=%d d100m=%u | speed=%.1f km/h odo=%.1f trip=%.1f brake=%d %s\n",
                      v.stateRaw, v.ready ? "READY" : "P", v.sport ? "SPORT" : "ECO", v.standDown,
                      v.dist100m, v.speedRaw * SPEED_KMH_PER_COUNT, v.odoRaw * ODO_KM_PER_COUNT,
                      tripStartSet ? (v.odoRaw - tripStartRaw) * ODO_KM_PER_COUNT : 0.0f, v.brake,
                      v.motionValid ? "ONLINE" : "OFFLINE");
    }
}

const VcuData &vcuData() { return v; }

void vcuResetTrip() {
    if (!haveOdo) return;
    setTripStart(v.odoRaw);
    Serial.println("[VCU] reset TRIP");
}

void vcuDebugText(char *buf, size_t size) {
    const uint32_t now = millis();
    snprintf(buf, size,
             "102 %s D0=%02X chống=%d\n"
             "  100m=%u\n"
             "201 %s v=%u (%04X)\n"
             "ODO=%u (%04X)\n"
             "phanh=%d",
             fresh(last102Ms, now) ? "OK" : "--", v.stateRaw, v.standDown, v.dist100m,
             fresh(last201Ms, now) ? "OK" : "--", v.speedRaw, v.speedRaw, v.odoRaw, v.odoRaw, v.brake);
}
