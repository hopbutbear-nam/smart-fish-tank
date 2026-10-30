// sensors.cpp: đọc DS18B20, pH, độ đục, mực nước; lọc; đánh dấu lỗi
#include "sensors.h"
#include <math.h>
#include <string.h>

SensorCal sensorsDefaultCal() {
  SensorCal c;
  c.ph_v686 = PH_V686;
  c.ph_v401 = PH_V401;
  c.turb_v_clear = TURB_V_CLEAR;
  c.turb_v_dirty = TURB_V_DIRTY;
  c.level_raw_dry = LEVEL_RAW_DRY;
  c.level_raw_full = LEVEL_RAW_FULL;
  return c;
}

// ---------------------------------------------------------------------------
// Hàm quy đổi thuần
// ---------------------------------------------------------------------------
float sensorsTrimmedMean(const float* v, uint8_t n) {
  if (n == 0) return NAN;
  float sum = v[0], lo = v[0], hi = v[0];
  for (uint8_t i = 1; i < n; i++) {
    sum += v[i];
    if (v[i] < lo) lo = v[i];
    if (v[i] > hi) hi = v[i];
  }
  if (n < 3) return sum / n;                 // quá ít mẫu: trung bình thường
  return (sum - lo - hi) / (n - 2);          // bỏ một mẫu nhỏ nhất và một mẫu lớn nhất (nhiễu xung)
}

bool sensorsTempValid(float c) {
  if (!(c == c)) return false;                               // NaN
  if (c == DS18B20_FAULT_C) return false;                    // -127: mất cảm biến
  if (c == DS18B20_POWERON_C) return false;                  // đúng 85,0: giá trị mặc định khi vừa bật nguồn (D4)
  return c >= TEMP_PLAUSIBLE_MIN && c <= TEMP_PLAUSIBLE_MAX;
}

bool sensorsAdcVoltOk(float v) {
  return v >= ADC_V_MIN_VALID && v <= ADC_V_MAX_VALID;       // NaN cũng cho kết quả false
}

float sensorsPhFromVolt(float v, const SensorCal& cal) {
  float dv = cal.ph_v686 - cal.ph_v401;
  if (fabsf(dv) < 0.01f) return NAN;                         // hai điểm hiệu chuẩn trùng nhau: không tính được
  return 6.86f + (v - cal.ph_v686) * (6.86f - 4.01f) / dv;
}

static float clampPct(float x) {
  if (!(x == x)) return 0.0f;
  return x < 0.0f ? 0.0f : (x > 100.0f ? 100.0f : x);
}

float sensorsTurbPct(float v, const SensorCal& cal) {
  float span = cal.turb_v_clear - cal.turb_v_dirty;
  if (fabsf(span) < 0.01f) return 0.0f;
  return clampPct((cal.turb_v_clear - v) / span * 100.0f);   // nước càng đục điện áp càng thấp
}

float sensorsLevelPct(float raw, const SensorCal& cal) {
  float span = (float)(cal.level_raw_full - cal.level_raw_dry);
  if (fabsf(span) < 1.0f) return 0.0f;                       // hiệu chuẩn hỏng: báo thấp để người dùng chú ý
  return clampPct((raw - cal.level_raw_dry) / span * 100.0f);
}

RiskInput sensorsToRiskInput(const SensorValues& s) {
  RiskInput r;
  r.temp_ok = s.temp_ok;
  r.temp_c = s.temp_ok ? s.temp_c : 0.0f;
  r.ph_ok = s.ph_ok;
  r.ph = s.ph_ok ? s.ph : 0.0f;
  r.turb_pct = s.turb_pct;
  r.level_pct = s.level_pct;
  return r;
}

// ---------------------------------------------------------------------------
// SensorHub
// ---------------------------------------------------------------------------
static bool due(uint32_t now, uint32_t target) {
  return (int32_t)(now - target) >= 0;       // đúng cả khi millis() quay vòng
}

enum { CH_PH = 0, CH_TURB = 1, CH_LEVEL = 2 };

SensorHub::SensorHub(SensorHal& hal, const SensorCal& cal) : hal_(hal), cal_(cal) {
  memset(&v_, 0, sizeof(v_));
  memset(buf_, 0, sizeof(buf_));
  nextTempMs_ = tempReadyMs_ = 0;
  nextSlowMs_ = lastSampleMs_ = levelOnMs_ = 0;
  tempWait_ = false;
  levelSettling_ = false;
  chan_ = -1;
  nSamp_ = 0;
}

void SensorHub::begin(uint32_t nowMs) {
  memset(&v_, 0, sizeof(v_));
  v_.temp_c = NAN;
  v_.ph = NAN;
  nextTempMs_ = nowMs;
  nextSlowMs_ = nowMs;
  tempWait_ = false;
  chan_ = -1;
  hal_.levelPower(false);
}

void SensorHub::tick(uint32_t now) {
  // --- nhiệt độ: yêu cầu rồi đọc sau TEMP_CONVERT_MS, không chờ ---
  if (!tempWait_) {
    if (due(now, nextTempMs_)) {
      hal_.tempRequest();
      tempWait_ = true;
      tempReadyMs_ = now + TEMP_CONVERT_MS;
      nextTempMs_ = now + TEMP_PERIOD_MS;
    }
  } else if (due(now, tempReadyMs_)) {
    float c = hal_.tempRead();
    tempWait_ = false;
    v_.temp_ok = sensorsTempValid(c);
    v_.temp_c = v_.temp_ok ? c : NAN;
    v_.temp_ready = true;
  }
  slowTick(now);
}

void SensorHub::startChannel(int8_t ch, uint32_t now) {
  chan_ = ch;
  nSamp_ = 0;
  if (ch == CH_LEVEL) {
    hal_.levelPower(true);                   // chỉ bật nguồn trong lúc đo để đầu dò không bị ăn mòn
    levelSettling_ = true;
    levelOnMs_ = now;
  } else {
    lastSampleMs_ = now - ADC_SAMPLE_GAP_MS; // cho phép lấy mẫu đầu tiên ngay lần tick sau
  }
}

void SensorHub::slowTick(uint32_t now) {
  if (chan_ < 0) {
    if (!due(now, nextSlowMs_)) return;
    nextSlowMs_ = now + SLOW_PERIOD_MS;
    startChannel(CH_PH, now);
    return;
  }
  if (chan_ == CH_LEVEL && levelSettling_) {
    if (!due(now, levelOnMs_ + LEVEL_PWR_SETTLE_MS)) return;   // chờ cảm biến ổn định
    levelSettling_ = false;
    lastSampleMs_ = now - LEVEL_SAMPLE_GAP_MS;
  }
  uint32_t gap = (chan_ == CH_LEVEL) ? LEVEL_SAMPLE_GAP_MS : ADC_SAMPLE_GAP_MS;
  if ((uint32_t)(now - lastSampleMs_) < gap) return;
  lastSampleMs_ = now;

  uint8_t total;
  if (chan_ == CH_PH)        { buf_[nSamp_++] = hal_.adcReadMv(PIN_PH);   total = ADC_SAMPLES; }
  else if (chan_ == CH_TURB) { buf_[nSamp_++] = hal_.adcReadMv(PIN_TURB); total = ADC_SAMPLES; }
  else                       { buf_[nSamp_++] = (float)hal_.adcReadRaw(PIN_LEVEL); total = LEVEL_SAMPLES; }
  if (nSamp_ >= total) finishChannel(now);
}

void SensorHub::finishChannel(uint32_t now) {
  float mean = sensorsTrimmedMean(buf_, nSamp_);
  if (chan_ == CH_PH) {
    float v = mean / 1000.0f * ADC_DIVIDER;                    // mV tại chân -> V của module
    float ph = sensorsPhFromVolt(v, cal_);
    bool ok = sensorsAdcVoltOk(v) && ph == ph && ph >= PH_PLAUSIBLE_MIN && ph <= PH_PLAUSIBLE_MAX;
    v_.ph_v = v;
    v_.ph_ok = ok;
    v_.ph = ok ? ph : NAN;
    startChannel(CH_TURB, now);
  } else if (chan_ == CH_TURB) {
    float v = mean / 1000.0f * ADC_DIVIDER;
    v_.turb_v = v;
    v_.turb_pct = sensorsTurbPct(v, cal_);
    startChannel(CH_LEVEL, now);
  } else {
    hal_.levelPower(false);
    v_.level_raw = (int)(mean + 0.5f);
    v_.level_pct = sensorsLevelPct(mean, cal_);
    v_.slow_ready = true;
    chan_ = -1;
  }
}

// ---------------------------------------------------------------------------
// HAL thật cho ESP32 (chỉ khi biên dịch cho Arduino)
// ---------------------------------------------------------------------------
#ifdef ARDUINO
#include <Arduino.h>

void Esp32SensorHal::begin() {
  dallas_.begin();
  dallas_.setResolution(12);
  dallas_.setWaitForConversion(false);       // không chặn: tempRequest() trả về ngay
  pinMode(PIN_LEVEL_PWR, OUTPUT);
  digitalWrite(PIN_LEVEL_PWR, LOW);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_PH, ADC_11db);
  analogSetPinAttenuation(PIN_TURB, ADC_11db);
  analogSetPinAttenuation(PIN_LEVEL, ADC_11db);
}

void Esp32SensorHal::tempRequest() {
  dallas_.requestTemperatures();
}

float Esp32SensorHal::tempRead() {
  return dallas_.getTempCByIndex(0);         // trả -127 nếu mất cảm biến
}

float Esp32SensorHal::adcReadMv(uint8_t pin) {
  return (float)analogReadMilliVolts(pin);
}

int Esp32SensorHal::adcReadRaw(uint8_t pin) {
  return analogRead(pin);
}

void Esp32SensorHal::levelPower(bool on) {
  digitalWrite(PIN_LEVEL_PWR, on ? HIGH : LOW);
}
#endif  // ARDUINO
