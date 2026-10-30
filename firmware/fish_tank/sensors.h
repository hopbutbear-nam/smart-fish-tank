#pragma once
// sensors.h: đọc DS18B20, pH, độ đục, mực nước; lọc; đánh dấu lỗi
//
// Thiết kế để thử được trên máy tính:
//   - SensorHal: giao diện phần cứng (đọc nhiệt độ, đọc ADC, bật nguồn cảm biến mực nước).
//       + Trên ESP32: Esp32SensorHal (cuối file, chỉ khi có ARDUINO).
//       + Trên máy tính: bài thử tự viết một HAL giả có kịch bản.
//   - SensorHub: bộ điều phối KHÔNG CHẶN (không delay): mỗi lần tick() chỉ làm một việc nhỏ.
//   - Các hàm quy đổi thuần (điện áp -> pH, độ đục %, mực nước %) nhận hằng số hiệu chuẩn qua SensorCal,
//     nên đo xong Giai đoạn 2 chỉ cần sửa config.h, không sửa code.
#include <stdint.h>
#include <stddef.h>
#include "config.h"
#include "risk.h"

struct SensorCal {               // hằng số hiệu chuẩn (bảng hiệu chuẩn Giai đoạn 2)
  float ph_v686, ph_v401;        // điện áp module pH trong dung dịch 6,86 và 4,01 (V)
  float turb_v_clear;            // điện áp module độ đục trong nước sạch (V)  -> 0 %
  float turb_v_dirty;            // điện áp trong nước rất đục (V)             -> 100 %
  int   level_raw_dry;           // giá trị ADC thô khi khô                    -> 0 %
  int   level_raw_full;          // giá trị ADC thô khi ngập tới mức đầy       -> 100 %
};
SensorCal sensorsDefaultCal();   // lấy từ config.h

struct SensorValues {
  bool  temp_ok;   float temp_c;                 // temp_c = NAN khi lỗi
  bool  ph_ok;     float ph;   float ph_v;       // ph = NAN khi lỗi; ph_v là điện áp module (V)
  float turb_v;    float turb_pct;               // điện áp module (V) và độ đục tương đối 0 đến 100 %
  int   level_raw; float level_pct;              // giá trị ADC thô và mực nước 0 đến 100 %
  bool  temp_ready;                              // đã có ít nhất một kết quả nhiệt độ
  bool  slow_ready;                              // đã có ít nhất một vòng pH, độ đục, mực nước
};

// ---- Hàm quy đổi thuần (thử trên máy tính) ----
float sensorsTrimmedMean(const float* v, uint8_t n);       // bỏ mẫu nhỏ nhất và lớn nhất rồi lấy trung bình
bool  sensorsTempValid(float c);                           // false nếu -127 (mất cảm biến), 85,0 (mặc định khi bật nguồn), NaN, ngoài dải
bool  sensorsAdcVoltOk(float v);                           // điện áp module trong dải hợp lệ (dây đứt hoặc chập thì sai)
float sensorsPhFromVolt(float v, const SensorCal& cal);    // pH = 6,86 + (V - V686) x (6,86 - 4,01) / (V686 - V401); NAN nếu hiệu chuẩn hỏng
float sensorsTurbPct(float v, const SensorCal& cal);       // 0 đến 100
float sensorsLevelPct(float raw, const SensorCal& cal);    // 0 đến 100
RiskInput sensorsToRiskInput(const SensorValues& s);       // đưa số liệu vào bộ tính risk

// ---- Giao diện phần cứng ----
class SensorHal {
 public:
  virtual ~SensorHal() {}
  virtual void  tempRequest() = 0;             // bắt đầu chuyển đổi DS18B20, KHÔNG chờ
  virtual float tempRead() = 0;                // kết quả (°C); -127 nếu mất cảm biến
  virtual float adcReadMv(uint8_t pin) = 0;    // mV tại chân GPIO (trước cầu chia áp)
  virtual int   adcReadRaw(uint8_t pin) = 0;   // giá trị thô 0 đến 4095
  virtual void  levelPower(bool on) = 0;       // cấp nguồn cho cảm biến mực nước (GPIO27)
};

// ---- Bộ điều phối không chặn ----
// Nhiệt độ: cứ TEMP_PERIOD_MS yêu cầu chuyển đổi, sau TEMP_CONVERT_MS thì đọc kết quả.
// pH, độ đục, mực nước: cứ SLOW_PERIOD_MS chạy một vòng; mỗi lần tick() lấy tối đa MỘT mẫu ADC.
class SensorHub {
 public:
  SensorHub(SensorHal& hal, const SensorCal& cal);
  void begin(uint32_t nowMs);
  void tick(uint32_t nowMs);                   // gọi mỗi vòng loop()
  const SensorValues& values() const { return v_; }
  bool ready() const { return v_.temp_ready && v_.slow_ready; }
 private:
  void slowTick(uint32_t nowMs);
  void startChannel(int8_t ch, uint32_t nowMs);
  void finishChannel(uint32_t nowMs);

  SensorHal&  hal_;
  SensorCal   cal_;
  SensorValues v_;
  // nhiệt độ
  uint32_t nextTempMs_, tempReadyMs_;
  bool     tempWait_;
  // kênh chậm
  uint32_t nextSlowMs_, lastSampleMs_, levelOnMs_;
  int8_t   chan_;                              // -1 = rảnh; 0 = pH, 1 = độ đục, 2 = mực nước
  uint8_t  nSamp_;
  bool     levelSettling_;
  float    buf_[ADC_SAMPLES];
};

#ifdef ARDUINO
#include <OneWire.h>
#include <DallasTemperature.h>
// HAL thật cho ESP32. Dùng: Esp32SensorHal hal; trong setup() gọi hal.begin().
class Esp32SensorHal : public SensorHal {
 public:
  Esp32SensorHal() : ow_(PIN_DS18B20), dallas_(&ow_) {}
  void  begin();
  void  tempRequest() override;
  float tempRead() override;
  float adcReadMv(uint8_t pin) override;
  int   adcReadRaw(uint8_t pin) override;
  void  levelPower(bool on) override;
 private:
  OneWire ow_;
  DallasTemperature dallas_;
};
#endif
