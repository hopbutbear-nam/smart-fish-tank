#pragma once
// alarm.h: còi 1300 Hz không chặn, mẫu kêu theo mức risk, tắt tiếng (E6, D9)
//
//   Mức 0: im lặng
//   Mức 1: 1 tiếng ngắn nhỏ (BUZZ_BEEP_MS) mỗi 60 giây
//   Mức 2: 2 tiếng nhỏ, cách nhau BUZZ_BEEP_MS, mỗi 30 giây
//   Mức 3: kêu to nhất, liên tục
//
// Lớp Alarm là máy trạng thái THUẦN: nhận (thời gian, risk) và trả về "còi bật hay tắt, độ rộng xung bao nhiêu %".
// Phần ghi ra chân còi (ledc PWM) nằm cuối file, chỉ khi biên dịch cho ESP32.
#include <stdint.h>
#include "config.h"

struct AlarmOut {
  bool    on;          // true: phát tiếng
  uint8_t dutyPct;     // độ rộng xung (0 đến 100); tiếng nhỏ dùng BUZZ_DUTY_QUIET_PCT, tiếng to nhất dùng BUZZ_DUTY_LOUD_PCT
};

// Đổi AlarmOut thành giá trị duty thô cho ledc (có xét BUZZ_ACTIVE_LOW)
uint32_t alarmRawDuty(const AlarmOut& o, uint8_t bits);

class Alarm {
 public:
  Alarm() { reset(); }
  void reset();
  // Từ config: buzzer_on và mute_min (phút). Gọi lại mỗi khi nhận config mới.
  void configure(bool buzzerOn, uint16_t muteMin);
  // Tắt tiếng mute_min phút cho SỰ CỐ HIỆN TẠI. Tự hủy khi risk tăng cao hơn mức lúc bấm, hoặc khi risk về 0.
  // Không làm gì nếu đang không có cảnh báo hoặc mute_min = 0.
  void mute(uint32_t nowMs);
  void unmute() { muted_ = false; }
  bool muted() const { return muted_; }
  // Gọi mỗi vòng loop() với risk hiện tại (0 đến 3). Mỗi khi risk đổi, chu kỳ kêu bắt đầu lại ngay.
  AlarmOut tick(uint32_t nowMs, uint8_t risk);
 private:
  bool     buzzerOn_;
  uint16_t muteMin_;
  uint8_t  level_;           // risk của lần tick trước
  uint32_t cycleStart_;      // thời điểm bắt đầu chu kỳ kêu hiện tại
  bool     muted_;
  uint8_t  muteRisk_;        // risk lúc bấm tắt tiếng
  uint32_t muteStart_, muteDur_;
};

#ifdef ARDUINO
void alarmBegin();                       // gắn chân còi vào ledc, tắt tiếng
void alarmApply(const AlarmOut& o);      // ghi ra còi (gọi mỗi vòng loop(), chỉ ghi khi có thay đổi)
#endif
