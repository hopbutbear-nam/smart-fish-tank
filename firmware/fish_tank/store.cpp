// store.cpp: cấu hình của bể và lưu vào NVS
#include "store.h"
#include <string.h>
#include <stdio.h>

AppConfig configDefaults() {
  AppConfig c;
  memset(&c, 0, sizeof(c));
  c.temp_min = DEF_TEMP_MIN;
  c.temp_max = DEF_TEMP_MAX;
  c.ph_min = DEF_PH_MIN;
  c.ph_max = DEF_PH_MAX;
  c.turb_max_pct = DEF_TURB_MAX_PCT;
  c.level_min_pct = DEF_LEVEL_MIN_PCT;
  c.level_danger_pct = DEF_LEVEL_DANGER_PCT;
  c.buzzer_on = DEF_BUZZER_ON;
  c.mute_min = DEF_MUTE_MIN;
  c.feed_ms = FEED_MS_DEFAULT;
  c.feed_count = 0;
  return c;
}

RiskThresholds configThresholds(const AppConfig& c) {
  RiskThresholds t;
  t.temp_min = c.temp_min;
  t.temp_max = c.temp_max;
  t.ph_min = c.ph_min;
  t.ph_max = c.ph_max;
  t.turb_max_pct = c.turb_max_pct;
  t.level_min_pct = c.level_min_pct;
  t.level_danger_pct = c.level_danger_pct;
  return t;
}

static bool fail(char* err, size_t errLen, const char* msg) {
  if (err && errLen) {
    strncpy(err, msg, errLen - 1);
    err[errLen - 1] = '\0';
  }
  return false;
}

// Mọi điều kiện viết dạng "phải đúng", nên giá trị NaN cũng bị từ chối (so sánh với NaN luôn sai).
bool configValidate(const AppConfig& c, char* err, size_t errLen) {
  if (!(c.temp_min < c.temp_max))
    return fail(err, errLen, "temp_min phai nho hon temp_max");
  if (!(c.ph_min < c.ph_max))
    return fail(err, errLen, "ph_min phai nho hon ph_max");
  if (!(c.temp_min >= TEMP_DANGER_MIN && c.temp_max <= TEMP_DANGER_MAX))
    return fail(err, errLen, "nguong nhiet do ngoai bien nguy hiem 18-33");
  if (!(c.ph_min >= PH_DANGER_MIN && c.ph_max <= PH_DANGER_MAX))
    return fail(err, errLen, "nguong pH ngoai bien nguy hiem 5.0-8.5");
  if (!(c.level_danger_pct >= 0 && c.level_danger_pct <= c.level_min_pct && c.level_min_pct <= 100))
    return fail(err, errLen, "can 0 <= level_danger <= level_min <= 100");
  if (!(c.turb_max_pct > 0 && c.turb_max_pct + TURB_L2_EXTRA <= 100))
    return fail(err, errLen, "turb_max_pct phai > 0 va + 15 <= 100");
  if (!(c.feed_ms >= FEED_MS_MIN && c.feed_ms <= FEED_MS_MAX))
    return fail(err, errLen, "feed_ms phai trong 100-3000");
  if (!(c.mute_min <= 1440))
    return fail(err, errLen, "mute_min phai trong 0-1440");
  if (!(c.feed_count <= FEED_MAX_TIMES))
    return fail(err, errLen, "qua so moc cho an");
  for (uint8_t i = 0; i < c.feed_count; i++) {
    if (c.feed_min[i] >= 24 * 60) return fail(err, errLen, "moc cho an ngoai 00:00-23:59");
    if (i > 0 && c.feed_min[i] <= c.feed_min[i - 1]) return fail(err, errLen, "moc cho an phai tang dan, khong trung");
  }
  return true;
}

bool parseHHMM(const char* s, uint16_t* minutesOut) {
  if (!s || strlen(s) != 5 || s[2] != ':') return false;
  for (int i = 0; i < 5; i++) {
    if (i == 2) continue;
    if (s[i] < '0' || s[i] > '9') return false;
  }
  int h = (s[0] - '0') * 10 + (s[1] - '0');
  int m = (s[3] - '0') * 10 + (s[4] - '0');
  if (h > 23 || m > 59) return false;
  if (minutesOut) *minutesOut = (uint16_t)(h * 60 + m);
  return true;
}

void formatHHMM(uint16_t minutes, char out[6]) {
  snprintf(out, 6, "%02u:%02u", (unsigned)(minutes / 60 % 24), (unsigned)(minutes % 60));
}

bool configParseFeedTimes(const char* const* times, uint8_t n, AppConfig& c, char* err, size_t errLen) {
  if (n > FEED_MAX_TIMES) return fail(err, errLen, "toi da 3 moc cho an");
  uint16_t tmp[FEED_MAX_TIMES];
  for (uint8_t i = 0; i < n; i++) {
    if (!parseHHMM(times[i], &tmp[i])) return fail(err, errLen, "moc cho an phai dang HH:MM");
  }
  // sắp tăng dần (chèn), sau đó từ chối nếu có hai mốc trùng nhau
  for (uint8_t i = 1; i < n; i++) {
    uint16_t v = tmp[i];
    int8_t j = (int8_t)i - 1;
    while (j >= 0 && tmp[j] > v) { tmp[j + 1] = tmp[j]; j--; }
    tmp[j + 1] = v;
  }
  for (uint8_t i = 1; i < n; i++) {
    if (tmp[i] == tmp[i - 1]) return fail(err, errLen, "co moc cho an bi trung");
  }
  c.feed_count = n;
  for (uint8_t i = 0; i < n; i++) c.feed_min[i] = tmp[i];
  return true;
}

// ---------------------------------------------------------------------------
// NVS (chỉ khi biên dịch cho Arduino/ESP32)
// ---------------------------------------------------------------------------
#ifdef ARDUINO
#include <Preferences.h>

#define NVS_NAMESPACE   "fishtank"
#define NVS_CFG_VERSION 1            // đổi số này nếu sau này đổi cấu trúc AppConfig

static Preferences g_prefs;

void storeBegin() {
  g_prefs.begin(NVS_NAMESPACE, false);
}

bool storeLoadConfig(AppConfig& out) {
  if (g_prefs.getUChar("cfg_v", 0) != NVS_CFG_VERSION) return false;
  if (g_prefs.getBytesLength("cfg") != sizeof(AppConfig)) return false;
  AppConfig tmp;
  if (g_prefs.getBytes("cfg", &tmp, sizeof(tmp)) != sizeof(tmp)) return false;
  char err[48];
  if (!configValidate(tmp, err, sizeof(err))) return false;   // dữ liệu hỏng thì dùng mặc định
  out = tmp;
  return true;
}

bool storeSaveConfig(const AppConfig& c) {
  if (g_prefs.putBytes("cfg", &c, sizeof(c)) != sizeof(c)) return false;
  g_prefs.putUChar("cfg_v", NVS_CFG_VERSION);
  return true;
}

uint8_t storeLoadFeedHistory(uint32_t out[FEED_MAX_PER_24H]) {
  uint8_t n = g_prefs.getUChar("fh_n", 0);
  if (n == 0 || n > FEED_MAX_PER_24H) return 0;
  size_t bytes = n * sizeof(uint32_t);
  if (g_prefs.getBytes("fh", out, bytes) != bytes) return 0;
  return n;
}

void storeSaveFeedHistory(const uint32_t* epochs, uint8_t n) {
  if (n > FEED_MAX_PER_24H) n = FEED_MAX_PER_24H;
  if (n) g_prefs.putBytes("fh", epochs, n * sizeof(uint32_t));
  g_prefs.putUChar("fh_n", n);
}
#endif  // ARDUINO
