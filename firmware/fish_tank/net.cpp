// net.cpp: WiFi, NTP, MQTT (TLS), publish, nhận lệnh và config, tự nối lại
#include "net.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "secrets.h"      // WIFI_SSID, WIFI_PASS, MQTT_PASS (không commit)
#include "store.h"
#include "feeder.h"
#include "sensors.h"
#include "risk.h"

// ---------------------------------------------------------------------------
// Hằng số riêng của net
// ---------------------------------------------------------------------------
#define NTP_WAIT_MS        15000UL   // chờ NTP tối đa ngần này lúc khởi động rồi vẫn đi tiếp (NTP vẫn chạy nền)
#define WIFI_RETRY_MS      20000UL   // quá ngần này chưa nối được WiFi thì gọi begin() lại
#define MQTT_SOCKET_TO_S   5         // giây: giới hạn chờ khi nối và khi đọc
#define TLS_HANDSHAKE_TO_S 8         // giây
#define FEED_QUEUE_LEN     4

// ---------------------------------------------------------------------------
// Trạng thái (một thể hiện duy nhất)
// ---------------------------------------------------------------------------
static WiFiClientSecure s_tls;
static PubSubClient     s_mqtt(s_tls);

static const AppConfig* s_cfgPtr = nullptr;

static bool     s_wifiWasUp = false;
static uint32_t s_wifiAttemptMs = 0;

static bool     s_ntpStarted = false;
static bool     s_ntpDone = false;
static uint32_t s_ntpStartMs = 0;

static bool     s_mqttWasUp = false;
static bool     s_bootSent = false;      // chỉ gửi event "boot" một lần mỗi lần bật nguồn
static uint32_t s_mqttNextMs = 0;
static uint32_t s_mqttBackoffMs = RECONNECT_MIN_MS;
static uint32_t s_lastMqttUpMs = 0;

static NetFeedCmd s_feedQ[FEED_QUEUE_LEN];
static uint8_t    s_feedHead = 0, s_feedCount = 0;

static AppConfig s_pendingCfg;
static bool      s_hasPendingCfg = false;
static bool      s_cfgRejected = false;
static char      s_cfgErr[64];

static bool due(uint32_t now, uint32_t target) {
  return (int32_t)(now - target) >= 0;       // đúng cả khi millis() quay vòng
}

// ---------------------------------------------------------------------------
// Tiện ích công khai nhỏ
// ---------------------------------------------------------------------------
bool netWifiUp() { return WiFi.status() == WL_CONNECTED; }
bool netMqttUp() { return s_mqtt.connected(); }

uint32_t netEpoch() {
  uint32_t t = (uint32_t)time(nullptr);
  return feedTimeValid(t) ? t : 0;           // trước khi NTP về, time() trả số rất nhỏ
}

uint32_t netMqttDownMs(uint32_t nowMs) {
  if (s_mqtt.connected()) return 0;
  return nowMs - s_lastMqttUpMs;
}

// ---------------------------------------------------------------------------
// Gửi event (JSON dựng bằng ArduinoJson để id từ web được escape đúng)
// Khớp onEvent() trong docs/app.js và fake_esp32.py: type, ts (giây), id, reason, portions, fw, level, msg (chuỗi).
//   type: boot | feed_done | feed_rejected | alarm | config_rejected
// ---------------------------------------------------------------------------
static bool sendEventDoc(JsonDocument& doc) {
  uint32_t ts = netEpoch();
  if (ts) doc["ts"] = ts;                    // chưa có giờ thì bỏ ts: web dùng giờ nhận
  char buf[256];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  if (n == 0 || n >= sizeof(buf)) return false;
  return s_mqtt.publish(TOPIC_EVENT, buf, false);
}

static bool publishEvent(const char* type, const char* id, const char* reason, int portions) {
  if (!s_mqtt.connected()) return false;
  JsonDocument doc;
  doc["type"] = type;
  if (id && id[0]) doc["id"] = id;
  if (reason) doc["reason"] = reason;
  if (portions > 0) doc["portions"] = portions;
  return sendEventDoc(doc);
}

bool netPublishFeedRejected(const char* id, const char* reason) {
  return publishEvent("feed_rejected", id, reason, 0);
}

bool netPublishFeedDone(const char* id, uint8_t portions) {
  return publishEvent("feed_done", id, nullptr, portions);
}

bool netPublishAlarm(const RiskResult& r) {
  if (!s_mqtt.connected() || r.risk == 0) return false;
  char msg[96];
  riskFlagsCsv(r.levels, msg, sizeof(msg));   // "temp_high,turb_high" (chuỗi, giống fake_esp32.py)
  JsonDocument doc;
  doc["type"] = "alarm";
  doc["level"] = (int)r.risk;
  doc["msg"] = msg;                           // msg còn sống tới khi serialize xong trong sendEventDoc
  return sendEventDoc(doc);
}

static bool publishBoot() {
  if (!s_mqtt.connected()) return false;
  JsonDocument doc;
  doc["type"] = "boot";
  doc["fw"] = FW_VERSION;
  return sendEventDoc(doc);
}

// ---------------------------------------------------------------------------
// Telemetry: dựng chuỗi bằng snprintf (không dùng ArduinoJson) để số thực in đúng "25.3",
// không bị "25.299999237" do ArduinoJson lưu float thành double.
// Khớp fake_esp32.py (publish_telemetry) và docs/app.js: v, ts (giây), temp_c, ph, turb_v, turb_pct, level_pct (số nguyên),
// risk, flags (MẢNG chuỗi; web dùng Array.isArray), rssi, uptime_s, fw. Cảm biến lỗi thì temp_c / ph = null.
// ---------------------------------------------------------------------------
// Ghi mảng JSON ["temp_high","turb_high"] vào buf ("[]" nếu không có cờ)
static void flagsJson(const RiskLevels& lv, char* buf, size_t len) {
  size_t n = 0;
  buf[n++] = '[';
  bool first = true;
  for (uint8_t i = 0; i < FL_COUNT; i++) {
    if (!lv.lv[i]) continue;
    const char* name = riskFlagName(i);
    size_t need = strlen(name) + 2 + (first ? 0 : 1);
    if (n + need + 2 > len) break;           // hết chỗ: dừng, không cắt dở
    if (!first) buf[n++] = ',';
    buf[n++] = '"';
    memcpy(buf + n, name, strlen(name));
    n += strlen(name);
    buf[n++] = '"';
    first = false;
  }
  buf[n++] = ']';
  buf[n] = '\0';
}

static void fmtNum(char* out, size_t len, bool ok, float v, int decimals) {
  if (ok && isfinite(v)) snprintf(out, len, "%.*f", decimals, (double)v);
  else snprintf(out, len, "null");
}

bool netPublishTelemetry(const SensorValues& s, const RiskResult& r) {
  if (!s_mqtt.connected()) return false;
  char temp[16], ph[16], turbV[16], turb[16], level[16], flags[96], tsPart[24];
  fmtNum(temp, sizeof(temp), s.temp_ok, s.temp_c, 1);
  fmtNum(ph, sizeof(ph), s.ph_ok, s.ph, 2);
  fmtNum(turbV, sizeof(turbV), true, s.turb_v, 2);
  fmtNum(turb, sizeof(turb), true, s.turb_pct, 0);       // số nguyên như fake_esp32.py
  fmtNum(level, sizeof(level), true, s.level_pct, 0);
  flagsJson(r.levels, flags, sizeof(flags));
  uint32_t ts = netEpoch();
  if (ts) snprintf(tsPart, sizeof(tsPart), "\"ts\":%lu,", (unsigned long)ts);
  else tsPart[0] = '\0';

  char buf[320];
  int n = snprintf(buf, sizeof(buf),
      "{\"v\":1,%s\"temp_c\":%s,\"ph\":%s,\"turb_v\":%s,\"turb_pct\":%s,\"level_pct\":%s,\"risk\":%u,\"flags\":%s,\"rssi\":%d,\"uptime_s\":%lu,\"fw\":\"%s\"}",
      tsPart, temp, ph, turbV, turb, level, (unsigned)r.risk, flags, (int)WiFi.RSSI(),
      (unsigned long)(millis() / 1000UL), FW_VERSION);
  if (n <= 0 || n >= (int)sizeof(buf)) return false;
  return s_mqtt.publish(TOPIC_TELEMETRY, buf, true);
}

// ---------------------------------------------------------------------------
// Nhận lệnh cho ăn: cmd/feed  {"id": "...", "portions": 1}
// ---------------------------------------------------------------------------
static void handleFeedMessage(const byte* payload, unsigned int len) {
  if (len == 0) return;                                   // payload rỗng = xóa retained, bỏ qua
  JsonDocument doc;
  if (deserializeJson(doc, (const char*)payload, len)) {
    Serial.println("[net] cmd/feed: JSON loi, bo qua");
    return;
  }
  if (!doc.is<JsonObject>()) return;
  JsonObjectConst o = doc.as<JsonObjectConst>();

  NetFeedCmd c;
  c.id[0] = '\0';
  c.portions = 1;
  JsonVariantConst idv = o["id"];
  if (idv.is<const char*>()) {
    strncpy(c.id, idv.as<const char*>(), sizeof(c.id) - 1);
    c.id[sizeof(c.id) - 1] = '\0';
  } else if (idv.is<long>()) {
    snprintf(c.id, sizeof(c.id), "%ld", idv.as<long>());
  }
  JsonVariantConst pv = o["portions"];
  if (pv.is<long>()) c.portions = pv.as<long>();          // sai kiểu thì giữ 1; feedClampPortions() kẹp 1..MAX_PORTIONS

  if (s_feedCount >= FEED_QUEUE_LEN) {
    Serial.println("[net] hang doi lenh cho an day, bo lenh");
    return;
  }
  s_feedQ[(s_feedHead + s_feedCount) % FEED_QUEUE_LEN] = c;
  s_feedCount++;
}

bool netTakeFeedCmd(NetFeedCmd& out) {
  if (s_feedCount == 0) return false;
  out = s_feedQ[s_feedHead];
  s_feedHead = (s_feedHead + 1) % FEED_QUEUE_LEN;
  s_feedCount--;
  return true;
}

// ---------------------------------------------------------------------------
// Nhận config: config  (retained, QoS 1). Các khóa phẳng, đều tùy chọn (gộp vào cấu hình cũ):
//   temp_min, temp_max, ph_min, ph_max, turb_max_pct, level_min_pct, level_danger_pct (số),
//   buzzer_on (bool), mute_min, feed_ms (số nguyên), feed_times (mảng "HH:MM", tối đa FEED_MAX_TIMES)
// Một giá trị sai kiểu hoặc ngoài biên thì TỪ CHỐI CẢ BẢN TIN (giống validate_config của fake_esp32.py).
// ---------------------------------------------------------------------------
static void rejectConfig(const char* msg) {
  strncpy(s_cfgErr, msg, sizeof(s_cfgErr) - 1);
  s_cfgErr[sizeof(s_cfgErr) - 1] = '\0';
  s_cfgRejected = true;
  Serial.printf("[net] config bi tu choi: %s\n", s_cfgErr);
}

static void readFloat(JsonObjectConst o, const char* key, float& dst, const char*& badKey) {
  JsonVariantConst v = o[key];
  if (v.isNull()) return;
  if (!v.is<float>()) { if (!badKey) badKey = key; return; }
  dst = v.as<float>();
}

static void readU16(JsonObjectConst o, const char* key, uint16_t& dst, const char*& badKey) {
  JsonVariantConst v = o[key];
  if (v.isNull()) return;
  if (!v.is<long>()) { if (!badKey) badKey = key; return; }
  long x = v.as<long>();
  if (x < 0 || x > 65535L) { if (!badKey) badKey = key; return; }   // chặn trước khi ép kiểu, tránh tràn số
  dst = (uint16_t)x;
}

static void handleConfigMessage(const byte* payload, unsigned int len) {
  if (len == 0) return;                                   // payload rỗng = xóa retained, bỏ qua
  JsonDocument doc;
  if (deserializeJson(doc, (const char*)payload, len)) { rejectConfig("JSON loi"); return; }
  if (!doc.is<JsonObject>()) { rejectConfig("config phai la object"); return; }
  JsonObjectConst o = doc.as<JsonObjectConst>();

  // gộp vào bản đang chờ (nếu main chưa kịp lấy) hoặc bản đang áp dụng
  AppConfig c = s_hasPendingCfg ? s_pendingCfg : (s_cfgPtr ? *s_cfgPtr : configDefaults());
  const char* badKey = nullptr;
  char err[64];

  readFloat(o, "temp_min", c.temp_min, badKey);
  readFloat(o, "temp_max", c.temp_max, badKey);
  readFloat(o, "ph_min", c.ph_min, badKey);
  readFloat(o, "ph_max", c.ph_max, badKey);
  readFloat(o, "turb_max_pct", c.turb_max_pct, badKey);
  readFloat(o, "level_min_pct", c.level_min_pct, badKey);
  readFloat(o, "level_danger_pct", c.level_danger_pct, badKey);
  readU16(o, "mute_min", c.mute_min, badKey);
  readU16(o, "feed_ms", c.feed_ms, badKey);

  JsonVariantConst bv = o["buzzer_on"];
  if (!bv.isNull()) {
    if (bv.is<bool>()) c.buzzer_on = bv.as<bool>();
    else if (!badKey) badKey = "buzzer_on";
  }

  if (badKey) {
    snprintf(err, sizeof(err), "sai kieu hoac ngoai dai: %s", badKey);
    rejectConfig(err);
    return;
  }

  JsonVariantConst ft = o["feed_times"];
  if (!ft.isNull()) {
    if (!ft.is<JsonArrayConst>()) { rejectConfig("feed_times phai la mang"); return; }
    JsonArrayConst arr = ft.as<JsonArrayConst>();
    if (arr.size() > FEED_MAX_TIMES) { rejectConfig("toi da 3 moc cho an"); return; }
    const char* ptrs[FEED_MAX_TIMES];
    uint8_t k = 0;
    for (JsonVariantConst e : arr) {
      if (!e.is<const char*>()) { rejectConfig("feed_times phai la chuoi HH:MM"); return; }
      ptrs[k++] = e.as<const char*>();
    }
    if (!configParseFeedTimes(ptrs, k, c, err, sizeof(err))) { rejectConfig(err); return; }
  }

  if (!configValidate(c, err, sizeof(err))) { rejectConfig(err); return; }

  s_pendingCfg = c;
  s_hasPendingCfg = true;
}

bool netTakeConfig(AppConfig& out) {
  if (!s_hasPendingCfg) return false;
  out = s_pendingCfg;
  s_hasPendingCfg = false;
  return true;
}

// ---------------------------------------------------------------------------
// Callback MQTT. Chỉ phân loại và đọc; KHÔNG publish trong này (PubSubClient dùng chung một bộ đệm
// cho gói nhận và gói gửi, publish trong callback sẽ làm hỏng payload đang đọc).
// ---------------------------------------------------------------------------
static void onMessage(char* topic, byte* payload, unsigned int len) {
  if (strcmp(topic, TOPIC_CMD_FEED) == 0) handleFeedMessage(payload, len);
  else if (strcmp(topic, TOPIC_CONFIG) == 0) handleConfigMessage(payload, len);
}

// ---------------------------------------------------------------------------
// Khởi tạo
// ---------------------------------------------------------------------------
void netBegin(const AppConfig* cfg) {
  s_cfgPtr = cfg;
  uint32_t now = millis();
  s_lastMqttUpMs = now;                       // đếm "mất MQTT" từ lúc bật nguồn
  s_mqttNextMs = now;
  s_mqttBackoffMs = RECONNECT_MIN_MS;

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  s_wifiAttemptMs = now;

  // Giai đoạn 3: bỏ qua kiểm tra chứng chỉ. Giai đoạn 6: thay bằng s_tls.setCACert(<nội dung certs/emqx-ca.pem>)
  // (khi đó bắt buộc đã có giờ NTP, nếu không bắt tay TLS sẽ lỗi và tự thử lại).
  s_tls.setInsecure();
  s_tls.setTimeout(MQTT_SOCKET_TO_S);
  s_tls.setHandshakeTimeout(TLS_HANDSHAKE_TO_S);

  s_mqtt.setServer(MQTT_HOST, MQTT_PORT);
  s_mqtt.setCallback(onMessage);
  s_mqtt.setKeepAlive(MQTT_KEEPALIVE_S);
  s_mqtt.setSocketTimeout(MQTT_SOCKET_TO_S);
  if (!s_mqtt.setBufferSize(MQTT_BUFFER_SIZE)) Serial.println("[net] LOI: khong cap duoc bo dem MQTT");

  Serial.printf("[net] bat dau noi WiFi: %s\n", WIFI_SSID);
}

// ---------------------------------------------------------------------------
// Nối MQTT một lần. Chặn tối đa vài giây (TLS).
// ---------------------------------------------------------------------------
static void tryConnectMqtt(uint32_t now) {
  Serial.printf("[net] noi MQTT %s:%d ...\n", MQTT_HOST, MQTT_PORT);
  // clean session = true: lệnh gửi lúc ESP32 offline sẽ không được giao bù (tránh cho ăn trễ).
  bool ok = s_mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS,
                           TOPIC_STATUS, 1 /*QoS Last Will*/, true /*retained*/, "offline", true);
  if (ok) {
    bool s1 = s_mqtt.subscribe(TOPIC_CMD_FEED, 1);
    bool s2 = s_mqtt.subscribe(TOPIC_CONFIG, 1);
    ok = s1 && s2;
    if (!ok) {
      Serial.println("[net] subscribe loi, ngat de thu lai");
      s_mqtt.disconnect();
    }
  }
  if (ok) {
    s_mqtt.publish(TOPIC_STATUS, "online", true);   // PubSubClient chỉ gửi QoS 0
    s_mqttBackoffMs = RECONNECT_MIN_MS;
    s_mqttWasUp = true;
    s_lastMqttUpMs = now;
    Serial.println("[net] MQTT da noi");
    if (!s_bootSent) { s_bootSent = publishBoot(); }
  } else {
    Serial.printf("[net] MQTT that bai (state=%d), thu lai sau %lu ms\n",
                  s_mqtt.state(), (unsigned long)s_mqttBackoffMs);
    s_mqttNextMs = now + s_mqttBackoffMs;
    s_mqttBackoffMs = s_mqttBackoffMs * 2;
    if (s_mqttBackoffMs > RECONNECT_MAX_MS) s_mqttBackoffMs = RECONNECT_MAX_MS;
  }
}

// ---------------------------------------------------------------------------
// Vòng lặp mạng
// ---------------------------------------------------------------------------
void netTick(uint32_t now, bool allowConnect) {
  // 1) WiFi
  if (WiFi.status() != WL_CONNECTED) {
    if (s_wifiWasUp) {
      s_wifiWasUp = false;
      s_wifiAttemptMs = now;                  // để setAutoReconnect tự thử trước
      Serial.println("[net] mat WiFi");
    }
    if (s_mqtt.connected()) s_mqtt.disconnect();
    if (s_mqttWasUp) { s_mqttWasUp = false; Serial.println("[net] mat MQTT"); }
    if ((uint32_t)(now - s_wifiAttemptMs) >= WIFI_RETRY_MS) {
      s_wifiAttemptMs = now;
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      Serial.println("[net] thu noi lai WiFi");
    }
    return;
  }
  if (!s_wifiWasUp) {
    s_wifiWasUp = true;
    s_mqttNextMs = now;                       // nối MQTT ngay khi có WiFi
    s_mqttBackoffMs = RECONNECT_MIN_MS;
    Serial.print("[net] WiFi da noi, IP ");
    Serial.println(WiFi.localIP());
  }

  // 2) NTP trước MQTT (chờ tối đa NTP_WAIT_MS lúc khởi động, sau đó vẫn đi tiếp; NTP tiếp tục chạy nền)
  if (!s_ntpStarted) {
    configTime(0, 0, NTP_SERVER);             // offset 0: epoch luôn là UTC; TZ_OFFSET_S do feeder.cpp tự cộng
    s_ntpStarted = true;
    s_ntpStartMs = now;
  }
  if (!s_ntpDone) {
    if (netEpoch() != 0) {
      s_ntpDone = true;
      Serial.printf("[net] da co gio NTP: %lu\n", (unsigned long)netEpoch());
    } else if ((uint32_t)(now - s_ntpStartMs) >= NTP_WAIT_MS) {
      s_ntpDone = true;
      Serial.println("[net] chua co gio NTP, van noi MQTT (lenh cho an se bi tu choi no_time)");
    } else {
      return;
    }
  }

  // 3) MQTT
  if (s_mqtt.connected()) {
    s_lastMqttUpMs = now;
    s_mqtt.loop();
    if (s_cfgRejected) {                      // báo lý do từ chối config (gửi ngoài callback)
      s_cfgRejected = false;
      publishEvent("config_rejected", nullptr, s_cfgErr, 0);
    }
  } else {
    if (s_mqttWasUp) { s_mqttWasUp = false; Serial.println("[net] mat MQTT"); }
    if (allowConnect && due(now, s_mqttNextMs)) tryConnectMqtt(now);
  }
}

#endif  // ARDUINO