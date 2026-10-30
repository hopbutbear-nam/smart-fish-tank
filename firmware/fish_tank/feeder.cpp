// feeder.cpp: luật cho ăn (E6, D5, D6, D7), lịch tự động, máy trạng thái quay servo
#include "feeder.h"
#include <string.h>

const char* feedRejectReason(FeedDecision d) {
  switch (d) {
    case FEED_NO_TIME:     return "no_time";       // MỚI so với mục E4: web cần thêm chữ tiếng Việt
    case FEED_BUSY:        return "busy";
    case FEED_TOO_SOON:    return "too_soon";
    case FEED_DAILY_LIMIT: return "daily_limit";
    default:               return nullptr;
  }
}

bool feedTimeValid(uint32_t nowEpoch) {
  return nowEpoch >= TIME_VALID_MIN_EPOCH;
}

uint8_t feedClampPortions(long requested) {
  if (requested < 1) return 1;
  if (requested > MAX_PORTIONS) return MAX_PORTIONS;
  return (uint8_t)requested;
}

uint32_t feedDurationMs(uint16_t feed_ms, uint8_t portions) {
  uint32_t d = (uint32_t)feed_ms * portions;
  return d > FEED_HARD_CAP_MS ? FEED_HARD_CAP_MS : d;
}

static void copyId(char* dst, size_t dstLen, const char* src) {
  if (!src) src = "";
  strncpy(dst, src, dstLen - 1);
  dst[dstLen - 1] = '\0';
}

// Tuổi của một mốc: không bao giờ âm (đồng hồ chỉnh lùi thì coi như vừa xảy ra, thận trọng)
static uint32_t ageOf(uint32_t nowEpoch, uint32_t t) {
  return nowEpoch >= t ? nowEpoch - t : 0;
}

// ---------------------------------------------------------------------------
// FeedPolicy
// ---------------------------------------------------------------------------
void FeedPolicy::reset() {
  memset(hist_, 0, sizeof(hist_));
  n_ = 0;
  lastId_[0] = '\0';
}

void FeedPolicy::load(const uint32_t* epochs, uint8_t n) {
  if (n > FEED_MAX_PER_24H) n = FEED_MAX_PER_24H;
  n_ = n;
  for (uint8_t i = 0; i < n; i++) hist_[i] = epochs[i];
}

FeedDecision FeedPolicy::checkLimits(uint32_t nowEpoch, bool busy) const {
  if (!feedTimeValid(nowEpoch)) return FEED_NO_TIME;
  if (busy) return FEED_BUSY;
  if (n_ > 0 && ageOf(nowEpoch, hist_[n_ - 1]) < FEED_MIN_INTERVAL_MS / 1000UL) return FEED_TOO_SOON;
  uint8_t recent = 0;
  for (uint8_t i = 0; i < n_; i++) {
    if (ageOf(nowEpoch, hist_[i]) < FEED_WINDOW_S) recent++;
  }
  if (recent >= FEED_MAX_PER_24H) return FEED_DAILY_LIMIT;
  return FEED_ACCEPT;
}

FeedDecision FeedPolicy::decideRemote(const char* id, uint32_t nowEpoch, bool busy) {
  if (id && id[0] && strcmp(id, lastId_) == 0) return FEED_IGNORE_DUP;   // chống gửi lặp (QoS 1 có thể giao 2 lần)
  copyId(lastId_, sizeof(lastId_), id);                                  // nhớ id kể cả khi bị từ chối
  return checkLimits(nowEpoch, busy);
}

FeedDecision FeedPolicy::decideScheduled(uint32_t nowEpoch, bool busy) const {
  return checkLimits(nowEpoch, busy);
}

void FeedPolicy::commit(uint32_t nowEpoch) {
  if (n_ == FEED_MAX_PER_24H) {                       // đầy: bỏ mốc cũ nhất
    for (uint8_t i = 1; i < n_; i++) hist_[i - 1] = hist_[i];
    n_--;
  }
  hist_[n_++] = nowEpoch;
}

int FeedPolicy::scheduleDue(const uint16_t* slotMin, uint8_t n, uint32_t nowEpoch) const {
  if (!feedTimeValid(nowEpoch)) return -1;
  uint32_t secOfDay = (nowEpoch + (uint32_t)TZ_OFFSET_S) % 86400UL;   // giây kể từ 00:00 giờ địa phương
  uint32_t midnight = nowEpoch - secOfDay;                            // 00:00 hôm nay (epoch UTC)
  for (uint8_t i = 0; i < n; i++) {
    // xét cả mốc của hôm nay và hôm qua (cửa sổ 5 phút có thể vắt qua nửa đêm)
    for (int back = 0; back <= 1; back++) {
      if (back == 1 && midnight < 86400UL) continue;
      uint32_t slotStart = midnight - (uint32_t)back * 86400UL + (uint32_t)slotMin[i] * 60UL;
      if (nowEpoch < slotStart || nowEpoch >= slotStart + FEED_SCHEDULE_GRACE_S) continue;
      bool alreadyFed = false;
      for (uint8_t k = 0; k < n_; k++) {
        if (hist_[k] >= slotStart) { alreadyFed = true; break; }
      }
      if (!alreadyFed) return (int)i;
    }
  }
  return -1;
}

// ---------------------------------------------------------------------------
// FeedRunner
// ---------------------------------------------------------------------------
void FeedRunner::reset() {
  running_ = false;
  startMs_ = 0;
  durMs_ = 0;
  portions_ = 0;
  id_[0] = '\0';
}

bool FeedRunner::start(uint32_t nowMs, uint32_t durMs, const char* id, uint8_t portions) {
  if (running_) return false;
  if (durMs > FEED_HARD_CAP_MS) durMs = FEED_HARD_CAP_MS;
  running_ = true;
  startMs_ = nowMs;
  durMs_ = durMs;
  portions_ = portions;
  copyId(id_, sizeof(id_), id);
  return true;
}

bool FeedRunner::tick(uint32_t nowMs) {
  if (!running_) return false;
  uint32_t elapsed = nowMs - startMs_;     // số học không dấu: đúng cả khi millis() quay vòng sau 49 ngày
  if (elapsed >= durMs_ || elapsed >= FEED_HARD_CAP_MS) {
    running_ = false;
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Servo (chỉ khi biên dịch cho Arduino/ESP32)
// ---------------------------------------------------------------------------
#ifdef ARDUINO
#include <ESP32Servo.h>

static Servo g_servo;
static int   g_lastUs = -1;

void feederBegin() {
  g_servo.setPeriodHertz(50);
  g_servo.attach(PIN_SERVO, SERVO_MIN_US, SERVO_MAX_US);
  g_servo.writeMicroseconds(SERVO_STOP_US);
  g_lastUs = SERVO_STOP_US;
}

void feederApply(const FeedRunner& r) {
  int us = r.servoUs();
  if (us != g_lastUs) {
    g_servo.writeMicroseconds(us);
    g_lastUs = us;
  }
}
#endif  // ARDUINO
