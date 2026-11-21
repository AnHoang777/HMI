#include "range.h"

#include <Arduino.h>
#include <Preferences.h>
#include <math.h>

namespace {

// Giá trị ban đầu khi chưa học: pack ~2,4 kWh, ~28 Wh/km → ~85 km cho 100 % → 0,85 km/%
constexpr float DEFAULT_KM_PER_PCT = 0.85f;
constexpr float PRIOR_PCT = 10;          // giá trị ban đầu nặng bằng 10 % pin đã học
constexpr float DECAY = 0.95f;           // mỗi mẫu mới, lịch sử cũ giảm trọng số 5 %
constexpr float MIN_KM_PER_PCT = 0.2f;
constexpr float MAX_KM_PER_PCT = 3.0f;
constexpr int MAX_STEP_PCT = 5;          // SOC giảm hơn 5 % một lần → coi là nhảy bất thường, bỏ qua

Preferences prefs;
float sumKm = DEFAULT_KM_PER_PCT * PRIOR_PCT;
float sumPct = PRIOR_PCT;
float lastSoc = NAN;

bool anchored = false;   // đã có mốc (SOC vừa giảm xuống một mức nguyên)
int anchorSoc = 0;
uint16_t anchorOdo = 0;

float rangeValue = NAN;

float kmPerPct() {
    const float k = sumKm / sumPct;
    return k < MIN_KM_PER_PCT ? MIN_KM_PER_PCT : (k > MAX_KM_PER_PCT ? MAX_KM_PER_PCT : k);
}

}  // namespace

void rangeBegin() {
    prefs.begin("range", false);
    sumKm = prefs.getFloat("km", sumKm);
    sumPct = prefs.getFloat("pct", sumPct);
    if (!(sumPct > 0) || !(sumKm > 0)) {
        sumKm = DEFAULT_KM_PER_PCT * PRIOR_PCT;
        sumPct = PRIOR_PCT;
    }
}

void rangeUpdate(bool socValid, float soc, bool odoValid, uint16_t odoRaw) {
    if (!socValid) {
        rangeValue = NAN;
        return;
    }
    rangeValue = soc * kmPerPct();

    if (!odoValid) return;
    const int s = (int)lroundf(soc);

    if (!anchored || s > anchorSoc) {
        // lần đầu, hoặc đang sạc → chờ lần SOC giảm tiếp theo mới đặt mốc
        if (anchored && s > anchorSoc) anchored = false;
        if (!isnan(lastSoc) && s < (int)lroundf(lastSoc) && !anchored) {
            anchored = true;
            anchorSoc = s;
            anchorOdo = odoRaw;
        }
        lastSoc = soc;
        return;
    }
    lastSoc = soc;
    if (s == anchorSoc) return;

    const int dPct = anchorSoc - s;
    const int dOdo = (int)odoRaw - (int)anchorOdo;
    anchorSoc = s;
    anchorOdo = odoRaw;
    if (dPct > MAX_STEP_PCT || dOdo < 0) return;

    sumKm = sumKm * DECAY + dOdo / 10.0f;
    sumPct = sumPct * DECAY + dPct;
    prefs.putFloat("km", sumKm);
    prefs.putFloat("pct", sumPct);
    Serial.printf("[RANGE] SOC -%d%% sau %.1f km → %.2f km/%%\n", dPct, dOdo / 10.0f, kmPerPct());
}

float rangeKm() { return rangeValue; }
float rangeKmPerPct() { return kmPerPct(); }
