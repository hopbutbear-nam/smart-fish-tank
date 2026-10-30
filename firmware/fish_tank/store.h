#pragma once
// store.h: cấu hình của bể (ngưỡng, cho ăn, còi) và lưu vào NVS (Preferences)
//
// Hai phần:
//   - Phần THUẦN (AppConfig, configValidate, parseHHMM...): không phụ thuộc Arduino, thử được trên máy tính.
//   - Phần NVS (storeBegin, storeLoad..., storeSave...): chỉ biên dịch khi có ARDUINO.
#include <stdint.h>
#include <stddef.h>
#include "config.h"
#include "risk.h"

struct AppConfig {
  float    temp_min, temp_max;
  float    ph_min, ph_max;
  float    turb_max_pct;
  float    level_min_pct, level_danger_pct;
  bool     buzzer_on;
  uint16_t mute_min;                    // phút tắt tiếng còi
  uint16_t feed_ms;                     // ms quay cho MỘT phần ăn
  uint8_t  feed_count;                  // số mốc cho ăn tự động (0 đến FEED_MAX_TIMES)
  uint16_t feed_min[FEED_MAX_TIMES];    // mỗi mốc = số phút kể từ 00:00 giờ địa phương, đã sắp tăng dần
};

AppConfig      configDefaults();
RiskThresholds configThresholds(const AppConfig& c);

// Kiểm tra TOÀN BỘ cấu hình (đã gộp giá trị mới vào giá trị cũ). Sai bất kỳ điều nào thì trả false và
// ghi lý do vào err; khi đó phải TỪ CHỐI cả bản tin config (giống validate_config của fake_esp32.py).
bool configValidate(const AppConfig& c, char* err, size_t errLen);

// "HH:MM" nghiêm ngặt (đúng 5 ký tự, 00:00 đến 23:59) -> số phút từ 00:00
bool parseHHMM(const char* s, uint16_t* minutesOut);
void formatHHMM(uint16_t minutes, char out[6]);

// Đọc danh sách giờ cho ăn (mảng chuỗi "HH:MM"), sắp tăng dần, từ chối nếu trùng, sai dạng hoặc quá
// FEED_MAX_TIMES mốc. Chỉ ghi vào c khi tất cả hợp lệ.
bool configParseFeedTimes(const char* const* times, uint8_t n, AppConfig& c, char* err, size_t errLen);

#ifdef ARDUINO
void    storeBegin();
bool    storeLoadConfig(AppConfig& out);                  // false nếu chưa có hoặc hỏng: dùng configDefaults()
bool    storeSaveConfig(const AppConfig& c);
uint8_t storeLoadFeedHistory(uint32_t out[FEED_MAX_PER_24H]);   // các mốc epoch (giây), cũ nhất trước
void    storeSaveFeedHistory(const uint32_t* epochs, uint8_t n);
#endif
