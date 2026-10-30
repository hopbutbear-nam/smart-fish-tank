#pragma once
// risk.h: tính risk (0 đến 3) và flags theo ngưỡng (mục E8 của kế hoạch)
//
// Hai lớp:
//   1) Hàm THUẦN riskEvaluate(): đầu vào là số liệu và ngưỡng, đầu ra là risk và flags.
//      Không phụ thuộc Arduino nên kiểm tra được trên máy tính (T3.11, tools/test-risk).
//   2) RiskFilter (D3, chống nhấp nháy): nâng mức khi điều kiện đúng 3 lần đánh giá liên tiếp,
//      hạ mức khi quay lại trong ngưỡng một khoảng nhỏ. Chạy trên firmware, gọi mỗi RISK_PERIOD_MS.
#include <stdint.h>
#include <stddef.h>

// Thứ tự cờ cũng là thứ tự khi ghép chuỗi flags
enum RiskFlag : uint8_t {
  FL_TEMP_HIGH = 0,
  FL_TEMP_LOW,
  FL_PH_HIGH,
  FL_PH_LOW,
  FL_TURB_HIGH,
  FL_LEVEL_LOW,
  FL_SENSOR_TEMP,
  FL_SENSOR_PH,
  FL_COUNT
};

struct RiskThresholds {      // lấy từ config (NVS); mức 2 đi theo ngưỡng mức 1
  float temp_min, temp_max;
  float ph_min, ph_max;
  float turb_max_pct;
  float level_min_pct;
  float level_danger_pct;
};

struct RiskInput {           // số liệu đã lọc và hiệu chuẩn
  bool  temp_ok;             // false nếu cảm biến nhiệt độ lỗi (-127, 85,0 sau khi bật nguồn)
  float temp_c;
  bool  ph_ok;               // false nếu cảm biến pH lỗi (ADC ngoài dải)
  float ph;
  float turb_pct;            // lỗi cảm biến độ đục và mực nước: chưa quy định (CẦN XÁC NHẬN)
  float level_pct;
};

struct RiskLevels {          // mức riêng của từng cờ (0 = không có cờ)
  uint8_t lv[FL_COUNT];
};

struct RiskResult {
  uint8_t    risk;           // 0 đến 3
  RiskLevels levels;         // cờ nào có lv > 0 thì nằm trong flags
};

const char* riskFlagName(uint8_t flag);
RiskThresholds riskDefaultThresholds();
RiskThresholds riskTighten(const RiskThresholds& th);          // thu hẹp ngưỡng một khoảng nhỏ (để hạ mức)

void    riskComputeLevels(const RiskInput& in, const RiskThresholds& th, RiskLevels& out);
uint8_t riskAggregate(const RiskLevels& lv);                   // luật tổng hợp nhiều cờ
RiskResult riskEvaluate(const RiskInput& in, const RiskThresholds& th);   // hàm thuần (T3.11)

// Ghép tên các cờ đang có, cách nhau dấu phẩy (ví dụ "temp_high,turb_high"). Trả về độ dài.
size_t riskFlagsCsv(const RiskLevels& lv, char* buf, size_t len);

class RiskFilter {
 public:
  RiskFilter() { reset(); }
  void reset();
  // Gọi một lần mỗi RISK_PERIOD_MS. Trả về mức đã qua chống nhấp nháy.
  RiskResult update(const RiskInput& in, const RiskThresholds& th);
 private:
  uint8_t cur_[FL_COUNT];    // mức đang áp dụng của từng cờ
  uint8_t upCnt_[FL_COUNT];  // số lần đánh giá liên tiếp có mức cao hơn mức đang áp dụng
  uint8_t upMin_[FL_COUNT];  // mức thấp nhất trong các lần đó (nâng thận trọng)
};
