// fish_tank.ino: chương trình chính ESP32 (Giai đoạn 3)
// Chỉ điều phối; không dùng delay() dài. Thứ tự mỗi vòng loop():
//   đọc cảm biến -> chấm risk (qua bộ lọc) -> còi -> cho ăn -> mạng -> telemetry -> watchdog
#include <Arduino.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "risk.h"
#include "store.h"
#include "feeder.h"
#include "sensors.h"
#include "alarm.h"
#include "net.h"

// ---------------------------------------------------------------------------
// Đối tượng toàn cục
// ---------------------------------------------------------------------------
static Esp32SensorHal g_hal;
static SensorHub      g_sensors(g_hal, sensorsDefaultCal());
static RiskFilter     g_filter;
static RiskResult     g_risk;            // khởi tạo trong setup(): risk 0, không cờ
static Alarm          g_alarm;
static FeedPolicy     g_policy;
static FeedRunner     g_runner;
static AppConfig      g_cfg;

static uint32_t g_nextRiskMs = 0;
static uint32_t g_nextTelemMs = 0;
static uint32_t g_nextSchedMs = 0;
static uint32_t g_nextLogMs = 0;
static uint8_t  g_prevRisk = 0;
static uint8_t  g_prevFlags = 0;          // mặt nạ bit các cờ đang có (để gửi telemetry ngay khi cờ đổi)
static uint32_t g_lastSchedRejectEpoch = 0;

static bool due(uint32_t now, uint32_t target) {
  return (int32_t)(now - target) >= 0;   // đúng cả khi millis() quay vòng
}

// ---------------------------------------------------------------------------
// Watchdog: core 2.x và 3.x khác API
// ---------------------------------------------------------------------------
static void wdtBegin() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_config_t cfg;
  cfg.timeout_ms = WDT_TIMEOUT_S * 1000;
  cfg.idle_core_mask = 0;                // chỉ canh loop(), không canh các task rảnh
  cfg.trigger_panic = true;              // treo quá hạn thì tự khởi động lại
  esp_task_wdt_reconfigure(&cfg);        // core 3.x đã khởi tạo sẵn watchdog
#else
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
#endif
  esp_task_wdt_add(NULL);                // thêm task loop() hiện tại
}

// ---------------------------------------------------------------------------
// Cho ăn
// ---------------------------------------------------------------------------
// Ghi nhận lần cho ăn NGAY khi bắt đầu quay (mất điện giữa chừng vẫn được tính vào hạn mức), rồi quay servo.
static void startFeed(const char* id, uint8_t portions, uint32_t epoch, uint32_t now) {
  g_policy.commit(epoch);
  storeSaveFeedHistory(g_policy.epochs(), g_policy.count());
  g_runner.start(now, feedDurationMs(g_cfg.feed_ms, portions), id, portions);
  feederApply(g_runner);
  Serial.printf("[feed] bat dau id=%s phan=%u thoi_gian=%lu ms\n", id, (unsigned)portions,
                (unsigned long)feedDurationMs(g_cfg.feed_ms, portions));
}

static void handleRemoteFeed(const NetFeedCmd& cmd, uint32_t epoch, uint32_t now) {
  FeedDecision d = g_policy.decideRemote(cmd.id, epoch, g_runner.busy());
  if (d == FEED_IGNORE_DUP) {
    Serial.printf("[feed] bo qua lenh trung id=%s\n", cmd.id);
    return;
  }
  if (d != FEED_ACCEPT) {
    const char* reason = feedRejectReason(d);
    Serial.printf("[feed] tu choi id=%s ly_do=%s\n", cmd.id, reason);
    netPublishFeedRejected(cmd.id, reason);
    return;
  }
  startFeed(cmd.id, feedClampPortions(cmd.portions), epoch, now);
}

static void handleSchedule(uint32_t epoch, uint32_t now) {
  if (g_runner.busy() || g_cfg.feed_count == 0) return;
  if (!due(now, g_nextSchedMs)) return;
  g_nextSchedMs = now + 1000;

  if (g_policy.scheduleDue(g_cfg.feed_min, g_cfg.feed_count, epoch) < 0) return;
  FeedDecision d = g_policy.decideScheduled(epoch, false);
  if (d == FEED_ACCEPT) {
    startFeed("schedule", 1, epoch, now);
    return;
  }
  // Bị từ chối (too_soon, daily_limit): mốc còn trong cửa sổ 5 phút nên sẽ thử lại mỗi giây;
  // chỉ báo event tối đa một lần mỗi cửa sổ để không làm ngập.
  if (g_lastSchedRejectEpoch == 0 || epoch - g_lastSchedRejectEpoch >= FEED_SCHEDULE_GRACE_S) {
    g_lastSchedRejectEpoch = epoch;
    const char* reason = feedRejectReason(d);
    Serial.printf("[feed] lich bi tu choi ly_do=%s\n", reason ? reason : "?");
    if (reason) netPublishFeedRejected("schedule", reason);
  }
}

// ---------------------------------------------------------------------------
// Log trạng thái mỗi 10 giây
// ---------------------------------------------------------------------------
static void logStatus() {
  const SensorValues& v = g_sensors.values();
  char flags[96];
  riskFlagsCsv(g_risk.levels, flags, sizeof(flags));   // chỉ để in log
  Serial.printf("[%lu s] T=%.1f pH=%.2f dUc=%.1f%% nuoc=%.1f%% | risk=%u [%s] | wifi=%d mqtt=%d gio=%d | tat_tieng=%d\n",
                (unsigned long)(millis() / 1000UL),
                v.temp_ok ? v.temp_c : NAN, v.ph_ok ? v.ph : NAN, v.turb_pct, v.level_pct,
                (unsigned)g_risk.risk, flags,
                netWifiUp(), netMqttUp(), netEpoch() != 0, g_alarm.muted());
}

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n=== fish_tank firmware %s (%s) ===\n", FW_VERSION, TANK_ID);
#if !CALIBRATION_DONE
  Serial.println("CANH BAO: CALIBRATION_DONE = 0, so pH / do duc / muc nuoc chi la gia tri tam (xem config.h)");
#endif

  // Đưa servo và còi về trạng thái an toàn trước mọi việc khác
  feederBegin();
  alarmBegin();

  // Cấu hình và lịch sử cho ăn từ NVS
  storeBegin();
  if (storeLoadConfig(g_cfg)) {
    Serial.println("[store] da nap cau hinh tu NVS");
  } else {
    g_cfg = configDefaults();
    Serial.println("[store] dung cau hinh mac dinh");
  }
  uint32_t hist[FEED_MAX_PER_24H];
  uint8_t nh = storeLoadFeedHistory(hist);
  g_policy.load(hist, nh);
  Serial.printf("[store] lich su cho an: %u lan\n", (unsigned)nh);

  g_alarm.configure(g_cfg.buzzer_on, g_cfg.mute_min);

  g_hal.begin();
  g_sensors.begin(millis());

  memset(&g_risk, 0, sizeof(g_risk));
  g_filter.reset();

  netBegin(&g_cfg);

  uint32_t now = millis();
  g_nextRiskMs = now + RISK_PERIOD_MS;
  g_nextTelemMs = now;
  g_nextSchedMs = now;
  g_nextLogMs = now + 10000;

  wdtBegin();
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
void loop() {
  uint32_t now = millis();
  uint32_t epoch = netEpoch();           // 0 nếu chưa có giờ

  // 1) Đọc cảm biến (không chặn)
  g_sensors.tick(now);

  // 2) Chấm risk mỗi RISK_PERIOD_MS, qua bộ lọc chống nhấp nháy. Chưa đọc xong lần đầu thì giữ risk 0.
  if (g_sensors.ready() && due(now, g_nextRiskMs)) {
    g_nextRiskMs = now + RISK_PERIOD_MS;
    g_risk = g_filter.update(sensorsToRiskInput(g_sensors.values()), configThresholds(g_cfg));
    uint8_t flagMask = 0;
    for (uint8_t i = 0; i < FL_COUNT; i++) if (g_risk.levels.lv[i]) flagMask |= (uint8_t)(1u << i);
    if (g_risk.risk != g_prevRisk || flagMask != g_prevFlags) {
      Serial.printf("[risk] %u -> %u\n", (unsigned)g_prevRisk, (unsigned)g_risk.risk);
      if (g_risk.risk > g_prevRisk) netPublishAlarm(g_risk);   // event "alarm" khi risk TĂNG (như fake_esp32.py)
      g_prevRisk = g_risk.risk;
      g_prevFlags = flagMask;
      g_nextTelemMs = now;               // gửi telemetry ngay khi risk hoặc cờ đổi
    }
  }

  // 3) Còi: chạy độc lập với mạng
  alarmApply(g_alarm.tick(now, g_risk.risk));

  // 4) Cho ăn: kết thúc lần quay, rồi lịch tự động
  bool feedDone = g_runner.tick(now);
  feederApply(g_runner);                 // servo về SERVO_STOP_US ngay khi hết giờ quay
  if (feedDone) {
    Serial.printf("[feed] xong id=%s\n", g_runner.id());
    netPublishFeedDone(g_runner.id(), g_runner.portions());
  }
  handleSchedule(epoch, now);

  // 5) Mạng. Không bắt đầu nối MQTT mới khi servo đang quay (mqtt.connect có thể chặn vài giây).
  netTick(now, !g_runner.busy());

  // 5a) Cấu hình mới từ web (đã gộp và đã validate trong net)
  AppConfig newCfg;
  if (netTakeConfig(newCfg)) {
    g_cfg = newCfg;
    if (!storeSaveConfig(g_cfg)) Serial.println("[store] LOI: khong luu duoc cau hinh vao NVS");
    g_alarm.configure(g_cfg.buzzer_on, g_cfg.mute_min);
    Serial.println("[config] da ap dung cau hinh moi");
  }

  // 5b) Lệnh cho ăn từ web
  NetFeedCmd cmd;
  while (netTakeFeedCmd(cmd)) handleRemoteFeed(cmd, epoch, now);

  // 6) Telemetry mỗi TELEMETRY_PERIOD_MS, và ngay khi risk hoặc cờ đổi (g_nextTelemMs = now ở trên)
  if (g_sensors.ready() && netMqttUp() && due(now, g_nextTelemMs)) {
    bool ok = netPublishTelemetry(g_sensors.values(), g_risk);
    g_nextTelemMs = now + (ok ? TELEMETRY_PERIOD_MS : 1000UL);
  }

  // 7) Log Serial
  if (due(now, g_nextLogMs)) {
    g_nextLogMs = now + 10000;
    logStatus();
  }

  // 8) Mất MQTT quá REBOOT_IF_NO_MQTT_MS thì tự khởi động lại (không làm khi servo đang quay)
  if (!g_runner.busy() && netMqttDownMs(now) >= REBOOT_IF_NO_MQTT_MS) {
    Serial.println("[wdt] mat MQTT qua lau, khoi dong lai");
    g_runner.reset();
    feederApply(g_runner);               // chắc chắn servo dừng
    AlarmOut off = {false, 0};
    alarmApply(off);                     // còi im
    Serial.flush();
    ESP.restart();
  }

  // 9) Watchdog
  esp_task_wdt_reset();
  delay(1);                              // nhường CPU cho WiFi và các task nền
}