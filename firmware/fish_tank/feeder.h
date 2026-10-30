#pragma once
// feeder.h: servo 360° cho ăn: luật chấp nhận lệnh, hạn mức, lịch, máy trạng thái quay
//
// Phần THUẦN (FeedPolicy, FeedRunner, hàm tiện ích): không phụ thuộc Arduino, thử được trên máy tính.
// Phần servo (feederBegin, feederApply): chỉ biên dịch khi có ARDUINO.
//
// Quy ước thời gian: nowEpoch là giây Unix (UTC) từ NTP; nowEpoch = 0 nghĩa là CHƯA có giờ.
// Mọi lần cho ăn (lệnh từ xa và lịch) đều cần có giờ vì hạn mức tính theo cửa sổ trượt 24 giờ (D5).
#include <stdint.h>
#include <stddef.h>
#include "config.h"

enum FeedDecision : uint8_t {
  FEED_ACCEPT = 0,       // được phép: bắt đầu quay rồi gọi commit()
  FEED_IGNORE_DUP,       // id trùng lệnh vừa xử lý: bỏ qua, KHÔNG gửi event
  FEED_NO_TIME,          // chưa đồng bộ NTP (D5)
  FEED_BUSY,             // đang quay
  FEED_TOO_SOON,         // chưa đủ 60 giây từ lần trước
  FEED_DAILY_LIMIT       // đã đủ 4 lần trong 24 giờ trượt
};

// Chuỗi "reason" gửi trong event feed_rejected; nullptr nếu không phải từ chối (ACCEPT, IGNORE_DUP)
const char* feedRejectReason(FeedDecision d);

#define TIME_VALID_MIN_EPOCH  1700000000UL   // trước mốc này (tháng 11/2023) coi là chưa có giờ
bool     feedTimeValid(uint32_t nowEpoch);
uint8_t  feedClampPortions(long requested);              // kẹp về 1 đến MAX_PORTIONS (giá trị sai: 1)
uint32_t feedDurationMs(uint16_t feed_ms, uint8_t portions);   // feed_ms x phần, không quá FEED_HARD_CAP_MS

class FeedPolicy {
 public:
  FeedPolicy() { reset(); }
  void reset();
  void load(const uint32_t* epochs, uint8_t n);       // nạp lịch sử từ NVS (cũ nhất trước)
  uint8_t count() const { return n_; }
  const uint32_t* epochs() const { return hist_; }    // để lưu NVS sau mỗi commit()

  // Lệnh từ xa. Thứ tự kiểm tra: id trùng, chưa có giờ, busy, too_soon, daily_limit.
  // Id rỗng thì không kiểm tra trùng.
  FeedDecision decideRemote(const char* id, uint32_t nowEpoch, bool busy);
  // Lịch tự động: không có id, vẫn chịu cùng hạn mức.
  FeedDecision decideScheduled(uint32_t nowEpoch, bool busy) const;
  // Ghi nhận một lần cho ăn đã được chấp nhận. Gọi NGAY khi bắt đầu quay (tính cả khi mất điện giữa chừng).
  void commit(uint32_t nowEpoch);

  // Mốc lịch nào đang đến giờ? Trả về chỉ số trong slotMin (phút từ 00:00 giờ địa phương), hoặc -1.
  // Mỗi mốc chạy trong cửa sổ FEED_SCHEDULE_GRACE_S sau giờ đặt; nếu từ giờ đặt đã có lần cho ăn nào
  // thì coi là xong (tránh cho ăn lặp sau khi khởi động lại). Lỡ cửa sổ thì bỏ qua, không cho ăn bù.
  int scheduleDue(const uint16_t* slotMin, uint8_t n, uint32_t nowEpoch) const;

 private:
  FeedDecision checkLimits(uint32_t nowEpoch, bool busy) const;
  uint32_t hist_[FEED_MAX_PER_24H];   // mốc epoch của các lần cho ăn gần nhất, cũ nhất trước
  uint8_t  n_;
  char     lastId_[32];
};

// Máy trạng thái quay servo, không dùng delay(). Gọi tick() mỗi vòng loop().
class FeedRunner {
 public:
  FeedRunner() { reset(); }
  void reset();
  bool busy() const { return running_; }
  // Bắt đầu quay durMs mili giây (bị cắt ở FEED_HARD_CAP_MS). Trả false nếu đang bận.
  bool start(uint32_t nowMs, uint32_t durMs, const char* id, uint8_t portions);
  // Trả true ĐÚNG MỘT LẦN khi vừa quay xong; lúc đó id() và portions() cho biết lần cho ăn vừa xong.
  bool tick(uint32_t nowMs);
  int  servoUs() const { return running_ ? SERVO_FEED_US : SERVO_STOP_US; }
  const char* id() const { return id_; }
  uint8_t portions() const { return portions_; }
 private:
  bool     running_;
  uint32_t startMs_, durMs_;
  uint8_t  portions_;
  char     id_[32];
};

#ifdef ARDUINO
void feederBegin();                         // gắn servo, đưa về SERVO_STOP_US
void feederApply(const FeedRunner& r);      // ghi xung ra servo khi thay đổi
#endif
