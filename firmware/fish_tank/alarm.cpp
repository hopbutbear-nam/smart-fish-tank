// alarm.cpp: còi 1300 Hz không chặn, ba mức âm thanh, tắt tiếng
#include "alarm.h"

uint32_t alarmRawDuty(const AlarmOut& o, uint8_t bits) {
  uint32_t maxv = (1UL << bits) - 1UL;
  uint32_t d = o.on ? (maxv * (uint32_t)o.dutyPct) / 100UL : 0UL;
#if BUZZ_ACTIVE_LOW
  d = maxv - d;                      // còi đảo logic: mức cao là im lặng
#endif
  return d;
}

void Alarm::reset() {
  buzzerOn_ = DEF_BUZZER_ON;
  muteMin_ = DEF_MUTE_MIN;
  level_ = 0;
  cycleStart_ = 0;
  muted_ = false;
  muteRisk_ = 0;
  muteStart_ = 0;
  muteDur_ = 0;
}

void Alarm::configure(bool buzzerOn, uint16_t muteMin) {
  buzzerOn_ = buzzerOn;
  muteMin_ = muteMin;
}

void Alarm::mute(uint32_t nowMs) {
  if (level_ == 0 || muteMin_ == 0) return;
  muted_ = true;
  muteRisk_ = level_;
  muteStart_ = nowMs;
  muteDur_ = (uint32_t)muteMin_ * 60000UL;
}

AlarmOut Alarm::tick(uint32_t now, uint8_t risk) {
  if (risk > 3) risk = 3;
  if (risk != level_) {              // risk đổi (lên hoặc xuống): bắt đầu chu kỳ kêu mới ngay
    level_ = risk;
    cycleStart_ = now;
  }

  if (muted_) {                      // tắt tiếng hủy khi: risk tăng, risk về 0, hoặc hết thời gian
    if (risk == 0 || risk > muteRisk_ || (uint32_t)(now - muteStart_) >= muteDur_) muted_ = false;
  }

  AlarmOut o;
  o.on = false;
  o.dutyPct = 0;
  if (!buzzerOn_ || muted_ || level_ == 0) return o;

  uint32_t elapsed = now - cycleStart_;          // số học không dấu: đúng cả khi millis() quay vòng
  switch (level_) {
    case 1: {
      uint32_t phase = elapsed % BUZZ_L1_PERIOD_MS;
      o.on = phase < BUZZ_BEEP_MS;               // 1 tiếng ngắn
      o.dutyPct = BUZZ_DUTY_QUIET_PCT;
      break;
    }
    case 2: {
      uint32_t phase = elapsed % BUZZ_L2_PERIOD_MS;
      o.on = phase < BUZZ_BEEP_MS ||                                   // tiếng thứ nhất
             (phase >= 2UL * BUZZ_BEEP_MS && phase < 3UL * BUZZ_BEEP_MS);  // nghỉ BUZZ_BEEP_MS rồi tiếng thứ hai
      o.dutyPct = BUZZ_DUTY_QUIET_PCT;
      break;
    }
    default:                                     // mức 3: to nhất, liên tục
      o.on = true;
      o.dutyPct = BUZZ_DUTY_LOUD_PCT;
      break;
  }
  return o;
}

// ---------------------------------------------------------------------------
// Ghi ra chân còi bằng PWM phần cứng (chỉ khi biên dịch cho ESP32)
// API ledc khác nhau giữa core 2.x và 3.x nên chọn theo phiên bản.
// ---------------------------------------------------------------------------
#ifdef ARDUINO
#include <Arduino.h>

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  #define BUZZ_CORE3 1
#else
  #define BUZZ_CORE3 0
#endif

static uint32_t g_lastRaw = 0xFFFFFFFFUL;

static void writeRaw(uint32_t duty) {
#if BUZZ_CORE3
  ledcWrite(PIN_BUZZER, duty);
#else
  ledcWrite(BUZZ_LEDC_CHANNEL, duty);
#endif
}

void alarmBegin() {
#if BUZZ_CORE3
  ledcAttach(PIN_BUZZER, BUZZ_FREQ_HZ, BUZZ_LEDC_BITS);
#else
  ledcSetup(BUZZ_LEDC_CHANNEL, BUZZ_FREQ_HZ, BUZZ_LEDC_BITS);
  ledcAttachPin(PIN_BUZZER, BUZZ_LEDC_CHANNEL);
#endif
  AlarmOut off = {false, 0};
  g_lastRaw = alarmRawDuty(off, BUZZ_LEDC_BITS);
  writeRaw(g_lastRaw);
}

void alarmApply(const AlarmOut& o) {
  uint32_t raw = alarmRawDuty(o, BUZZ_LEDC_BITS);
  if (raw != g_lastRaw) {
    writeRaw(raw);
    g_lastRaw = raw;
  }
}
#endif  // ARDUINO
