#pragma once
// net.h: WiFi, NTP, MQTT (TLS), publish, nhận lệnh và config, tự nối lại
//
// Toàn bộ chỉ biên dịch khi có ARDUINO (giống phần phần cứng của các module khác).
// KHÔNG chặn lâu: chỉ có mqtt.connect() qua TLS là chặn vài giây, và netTick() không gọi nó
// khi allowConnect = false (fish_tank.ino truyền false lúc servo đang quay).
#include <stdint.h>
#include <stddef.h>
#include "config.h"

#ifdef ARDUINO
struct AppConfig;
struct SensorValues;
struct RiskResult;

struct NetFeedCmd {
  char id[32];        // rỗng nếu lệnh không có id (khi đó không kiểm tra trùng)
  long portions;      // số phần yêu cầu, CHƯA kẹp; feedClampPortions() sẽ kẹp
};

// cfg: con trỏ tới cấu hình đang áp dụng (để gộp bản config từng phần). Gọi một lần trong setup().
void netBegin(const AppConfig* cfg);

// Gọi mỗi vòng loop(). allowConnect = false thì không bắt đầu nối MQTT mới (tránh chặn lúc servo quay).
void netTick(uint32_t nowMs, bool allowConnect);

bool     netWifiUp();
bool     netMqttUp();
uint32_t netEpoch();                     // giây Unix (UTC); 0 nếu chưa có giờ NTP
uint32_t netMqttDownMs(uint32_t nowMs);  // đã mất MQTT liên tục bao lâu (tính từ lúc bật nguồn nếu chưa nối được lần nào)

// Lấy lệnh / config đang chờ. Trả true và điền out nếu có. Gọi lặp tới khi false.
bool netTakeFeedCmd(NetFeedCmd& out);
bool netTakeConfig(AppConfig& out);      // đã gộp vào cấu hình cũ và đã qua configValidate()

// Gửi lên broker. Trả false nếu chưa nối MQTT hoặc gửi lỗi.
bool netPublishTelemetry(const SensorValues& s, const RiskResult& r);   // QoS 0, retained
bool netPublishFeedRejected(const char* id, const char* reason);        // QoS 0
bool netPublishFeedDone(const char* id, uint8_t portions);              // QoS 0
bool netPublishAlarm(const RiskResult& r);                              // event "alarm" (level = risk, msg = các cờ); false nếu risk = 0
#endif