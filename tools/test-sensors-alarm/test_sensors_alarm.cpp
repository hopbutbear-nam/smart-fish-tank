// test_sensors_alarm.cpp: thử phần THUẦN của sensors.cpp và alarm.cpp trên máy tính (chưa cần ESP32).
// Biên dịch từ thư mục gốc của kho (một dòng):
//   g++ -std=c++11 -Wall -I firmware/fish_tank tools/test-sensors-alarm/test_sensors_alarm.cpp firmware/fish_tank/sensors.cpp firmware/fish_tank/alarm.cpp firmware/fish_tank/risk.cpp -o tools/test-sensors-alarm/test_sensors_alarm.exe
// Chạy: chcp 65001   rồi   .\tools\test-sensors-alarm\test_sensors_alarm.exe
// Liên quan bài thử: T3.3, T3.4, T3.9 (phần logic; số đo thật và tiếng còi thật thử khi có ESP32).
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sensors.h"
#include "alarm.h"

static int total = 0, failures = 0;

static void expect(bool cond, const char* name) {
  total++;
  if (!cond) failures++;
  printf("%s  %s\n", cond ? "PASS" : "FAIL", name);
}
static bool near(float a, float b, float tol) { return fabsf(a - b) <= tol; }

// Hiệu chuẩn dùng riêng cho bài thử (không phụ thuộc số tạm trong config.h)
static SensorCal testCal() {
  SensorCal c;
  c.ph_v686 = 2.5f;  c.ph_v401 = 3.0f;
  c.turb_v_clear = 4.0f;  c.turb_v_dirty = 2.5f;
  c.level_raw_dry = 0;  c.level_raw_full = 2500;
  return c;
}

// HAL giả: có kịch bản, đếm số lần gọi, đo thời gian bật nguồn cảm biến mực nước
struct FakeHal : SensorHal {
  uint32_t now = 0;
  float tempC = 26.5f;
  float phMv = 1250.0f;      // chân GPIO: 1250 mV -> module 2,50 V -> pH 6,86
  float turbMv = 1625.0f;    // module 3,25 V -> 50 %
  int   levelRawVal = 1250;  // 50 %
  int   spikeAtPhCall = -1;  // lần gọi ADC pH thứ n (đếm từ 1) trả về giá trị đột biến
  float spikeMv = 3000.0f;
  int tempRequests = 0, tempReads = 0, phReads = 0, turbReads = 0, levelReads = 0, powerOns = 0;
  int callsThisTick = 0, maxCallsPerTick = 0;
  bool levelOn = false; uint32_t levelOnAt = 0, maxOnMs = 0;

  void beginTick(uint32_t t) { now = t; callsThisTick = 0; }
  void endTick() { if (callsThisTick > maxCallsPerTick) maxCallsPerTick = callsThisTick; }

  void tempRequest() override { tempRequests++; callsThisTick++; }
  float tempRead() override { tempReads++; callsThisTick++; return tempC; }
  float adcReadMv(uint8_t pin) override {
    callsThisTick++;
    if (pin == PIN_PH) {
      phReads++;
      if (phReads == spikeAtPhCall) return spikeMv;
      return phMv;
    }
    turbReads++;
    return turbMv;
  }
  int adcReadRaw(uint8_t) override { levelReads++; callsThisTick++; return levelRawVal; }
  void levelPower(bool on) override {
    if (on && !levelOn) { levelOn = true; levelOnAt = now; powerOns++; }
    if (!on && levelOn) { levelOn = false; uint32_t d = now - levelOnAt; if (d > maxOnMs) maxOnMs = d; }
  }
};

// Chạy hub từ startMs, trong durMs mili giây, mỗi bước 1 ms. Trả về mốc đầu tiên ready() (hoặc 0xFFFFFFFF).
static uint32_t runHub(FakeHal& hal, SensorHub& hub, uint32_t startMs, uint32_t durMs) {
  uint32_t firstReady = 0xFFFFFFFFu;
  hub.begin(startMs);
  for (uint32_t i = 0; i <= durMs; i++) {
    uint32_t t = startMs + i;
    hal.beginTick(t);
    hub.tick(t);
    hal.endTick();
    if (firstReady == 0xFFFFFFFFu && hub.ready()) firstReady = i;
  }
  return firstReady;
}

static AlarmOut tk(Alarm& a, uint32_t t, uint8_t risk) { return a.tick(t, risk); }

int main() {
  printf("=== sensors: hàm quy đổi ===\n");
  { float a[5] = {1, 2, 3, 4, 100};
    expect(near(sensorsTrimmedMean(a, 5), 3.0f, 0.001f), "Trung bình cắt bỏ nhiễu xung: {1,2,3,4,100} -> 3"); }
  { float a[4] = {5, 5, 5, 5};
    expect(near(sensorsTrimmedMean(a, 4), 5.0f, 0.001f), "Các mẫu bằng nhau -> đúng giá trị đó"); }
  { float a[2] = {2, 4};
    expect(near(sensorsTrimmedMean(a, 2), 3.0f, 0.001f), "Chỉ 2 mẫu: trung bình thường"); }

  expect(sensorsTempValid(26.4f), "26,4 °C hợp lệ");
  expect(!sensorsTempValid(-127.0f), "-127 (mất cảm biến): lỗi");
  expect(!sensorsTempValid(85.0f), "Đúng 85,0 (mặc định khi bật nguồn): lỗi");
  expect(sensorsTempValid(84.9f), "84,9 °C không bị loại nhầm là 85,0");
  expect(!sensorsTempValid(0.0f / 0.0f), "NaN: lỗi");
  expect(!sensorsTempValid(130.0f), "130 °C (ngoài dải DS18B20): lỗi");

  expect(!sensorsAdcVoltOk(0.0f), "Điện áp 0 V (dây đứt, chập): lỗi");
  expect(sensorsAdcVoltOk(2.5f), "Điện áp 2,5 V: hợp lệ");
  expect(!sensorsAdcVoltOk(5.3f), "Điện áp 5,3 V: lỗi");
  expect(!sensorsAdcVoltOk(0.0f / 0.0f), "Điện áp NaN: lỗi");

  { SensorCal c = testCal();
    expect(near(sensorsPhFromVolt(2.5f, c), 6.86f, 0.001f), "pH tại V686 = 6,86");
    expect(near(sensorsPhFromVolt(3.0f, c), 4.01f, 0.001f), "pH tại V401 = 4,01");
    expect(near(sensorsPhFromVolt(2.4f, c), 7.43f, 0.001f), "pH tại 2,40 V = 7,43 (ngoại suy theo đường thẳng)");
    SensorCal bad = c; bad.ph_v401 = bad.ph_v686;
    expect(!(sensorsPhFromVolt(2.5f, bad) == sensorsPhFromVolt(2.5f, bad)), "Hai điểm hiệu chuẩn trùng nhau: NaN (báo lỗi)");

    expect(near(sensorsTurbPct(4.0f, c), 0.0f, 0.01f), "Độ đục: nước sạch = 0 %");
    expect(near(sensorsTurbPct(2.5f, c), 100.0f, 0.01f), "Độ đục: rất đục = 100 %");
    expect(near(sensorsTurbPct(3.25f, c), 50.0f, 0.01f), "Độ đục: giữa = 50 %");
    expect(near(sensorsTurbPct(4.5f, c), 0.0f, 0.01f) && near(sensorsTurbPct(2.0f, c), 100.0f, 0.01f), "Độ đục: ngoài dải bị kẹp 0 đến 100");

    expect(near(sensorsLevelPct(0, c), 0.0f, 0.01f), "Mực nước: khô = 0 %");
    expect(near(sensorsLevelPct(2500, c), 100.0f, 0.01f), "Mực nước: đầy = 100 %");
    expect(near(sensorsLevelPct(1250, c), 50.0f, 0.01f), "Mực nước: giữa = 50 %");
    expect(near(sensorsLevelPct(-10, c), 0.0f, 0.01f) && near(sensorsLevelPct(3000, c), 100.0f, 0.01f), "Mực nước: ngoài dải bị kẹp");
    SensorCal bl = c; bl.level_raw_full = bl.level_raw_dry;
    expect(near(sensorsLevelPct(1000, bl), 0.0f, 0.01f), "Hiệu chuẩn mực nước hỏng: báo 0 % (để người dùng chú ý)");
  }
  { SensorCal d = sensorsDefaultCal();
    expect(d.ph_v686 == PH_V686 && d.level_raw_full == LEVEL_RAW_FULL, "sensorsDefaultCal() lấy từ config.h"); }

  printf("\n=== sensors: bộ điều phối không chặn (HAL giả) ===\n");
  {
    FakeHal hal; SensorHub hub(hal, testCal());
    uint32_t firstReady = runHub(hal, hub, 0, 12000);
    const SensorValues& v = hub.values();
    expect(firstReady <= 1000, "Có đủ số liệu đầu tiên trong vòng 1 giây");
    expect(v.temp_ok && near(v.temp_c, 26.5f, 0.01f), "Nhiệt độ đọc được 26,5 °C");
    expect(v.ph_ok && near(v.ph, 6.86f, 0.01f) && near(v.ph_v, 2.5f, 0.001f), "pH 6,86 (2,50 V)");
    expect(near(v.turb_v, 3.25f, 0.001f) && near(v.turb_pct, 50.0f, 0.01f), "Độ đục 50 % (3,25 V)");
    expect(v.level_raw == 1250 && near(v.level_pct, 50.0f, 0.01f), "Mực nước 50 % (thô 1250)");
    expect(hal.tempRequests >= 6 && hal.tempRequests <= 7, "Nhiệt độ: khoảng 2 giây một lần (6 đến 7 lần trong 12 giây)");
    expect(hal.phReads == 3 * ADC_SAMPLES && hal.turbReads == 3 * ADC_SAMPLES, "pH và độ đục: 3 vòng x 32 mẫu (5 giây một vòng)");
    expect(hal.levelReads == 3 * LEVEL_SAMPLES && hal.powerOns == 3, "Mực nước: 3 vòng, mỗi vòng bật nguồn một lần");
    expect(!hal.levelOn, "Xong vòng đo: nguồn cảm biến mực nước đã tắt");
    expect(hal.maxOnMs <= 40, "Mỗi lần bật nguồn cảm biến mực nước không quá 40 ms");
    expect(hal.maxCallsPerTick <= 1, "KHÔNG CHẶN: mỗi lần tick() gọi phần cứng tối đa một lần");
  }
  {
    FakeHal hal; hal.spikeAtPhCall = 5;          // một mẫu đột biến 3000 mV giữa 32 mẫu
    SensorHub hub(hal, testCal());
    runHub(hal, hub, 0, 1500);
    expect(near(hub.values().ph, 6.86f, 0.01f), "Một mẫu nhiễu xung bị loại: pH vẫn 6,86");
  }
  {
    FakeHal hal; hal.tempC = -127.0f; hal.phMv = 0.0f;
    hal.turbMv = 1910.0f;                       // module 3,82 V -> độ đục 12 % (bình thường), để chỉ còn hai cờ lỗi cảm biến
    SensorHub hub(hal, testCal());
    runHub(hal, hub, 0, 1500);
    const SensorValues& v = hub.values();
    expect(!v.temp_ok && !(v.temp_c == v.temp_c), "Rút DS18B20 (-127): temp_ok = false, temp_c = NaN (T3.4)");
    expect(!v.ph_ok && !(v.ph == v.ph), "Đầu dò pH 0 V: ph_ok = false, ph = NaN");
    RiskInput ri = sensorsToRiskInput(v);
    RiskResult rr = riskEvaluate(ri, riskDefaultThresholds());
    char flags[64]; riskFlagsCsv(rr.levels, flags, sizeof(flags));
    expect(rr.risk == 1 && strcmp(flags, "sensor_temp,sensor_ph") == 0, "Lỗi cảm biến đi vào risk: risk 1, cờ sensor_temp và sensor_ph");
  }
  {
    FakeHal hal; hal.tempC = 85.0f;
    SensorHub hub(hal, testCal());
    runHub(hal, hub, 0, 1500);
    expect(!hub.values().temp_ok, "DS18B20 đọc đúng 85,0: coi là lỗi (D4)");
  }
  {
    FakeHal hal; SensorHub hub(hal, testCal());
    uint32_t start = 0xFFFFFC00u;                 // millis() sắp quay vòng
    uint32_t fr = runHub(hal, hub, start, 3000);
    expect(fr <= 1000 && near(hub.values().ph, 6.86f, 0.01f) && hub.values().temp_ok, "millis() quay vòng giữa chừng: vẫn đọc bình thường");
  }
  {
    FakeHal hal; SensorHub hub(hal, testCal());
    runHub(hal, hub, 0, 1500);
    hal.tempC = 31.0f;
    for (uint32_t t = 1501; t <= 4000; t++) { hal.beginTick(t); hub.tick(t); hal.endTick(); }
    expect(near(hub.values().temp_c, 31.0f, 0.01f), "Nước ấm lên: nhiệt độ cập nhật trong vòng 2,5 giây (T3.3)");
  }

  printf("\n=== alarm: ba mức âm thanh (D9) ===\n");
  {
    Alarm a;
    expect(!tk(a, 0, 0).on && !tk(a, 4000, 0).on, "Mức 0: im lặng");
    AlarmOut o = tk(a, 5000, 1);
    expect(o.on && o.dutyPct == BUZZ_DUTY_QUIET_PCT, "Mức 1: kêu ngay khi risk lên 1, tiếng nhỏ");
    expect(tk(a, 5199, 1).on, "Mức 1: tiếng kéo dài 200 ms");
    expect(!tk(a, 5200, 1).on, "Mức 1: hết tiếng sau 200 ms");
    expect(!tk(a, 64999, 1).on, "Mức 1: im lặng suốt gần 60 giây");
    expect(tk(a, 65000, 1).on, "Mức 1: tiếng kế tiếp đúng 60 giây sau");
  }
  {
    Alarm a;
    AlarmOut o = tk(a, 10000, 2);
    expect(o.on && o.dutyPct == BUZZ_DUTY_QUIET_PCT, "Mức 2: kêu ngay, tiếng nhỏ");
    expect(tk(a, 10199, 2).on, "Mức 2: tiếng thứ nhất 200 ms");
    expect(!tk(a, 10200, 2).on && !tk(a, 10399, 2).on, "Mức 2: nghỉ 200 ms");
    expect(tk(a, 10400, 2).on && tk(a, 10599, 2).on, "Mức 2: tiếng thứ hai 200 ms");
    expect(!tk(a, 10600, 2).on && !tk(a, 39999, 2).on, "Mức 2: im lặng đến hết 30 giây");
    expect(tk(a, 40000, 2).on, "Mức 2: lặp lại đúng 30 giây sau");
  }
  {
    Alarm a;
    AlarmOut o = tk(a, 1000, 3);
    expect(o.on && o.dutyPct == BUZZ_DUTY_LOUD_PCT, "Mức 3: kêu to nhất");
    expect(tk(a, 1500, 3).on && tk(a, 31000, 3).on && tk(a, 999999, 3).on, "Mức 3: kêu liên tục");
    expect(!tk(a, 1000000, 0).on, "Risk về 0: im ngay");
  }
  {
    Alarm a;
    tk(a, 0, 1);
    expect(tk(a, 30000, 2).on, "Risk tăng 1 lên 2: kêu ngay, không đợi hết chu kỳ cũ");
    expect(tk(a, 50000, 3).on, "Risk tăng 2 lên 3: kêu ngay");
    expect(tk(a, 60000, 2).on, "Risk giảm 3 xuống 2: bắt đầu chu kỳ mức 2 ngay");
  }
  {
    Alarm a; a.configure(false, 30);
    expect(!tk(a, 0, 3).on, "buzzer_on = false: im lặng cả ở mức 3");
  }
  {
    Alarm a; a.configure(true, 30);
    tk(a, 0, 0);
    a.mute(0);
    expect(!a.muted(), "Không có cảnh báo: lệnh tắt tiếng không có tác dụng");
  }
  {
    Alarm a; a.configure(true, 0);
    tk(a, 0, 2); a.mute(100);
    expect(!a.muted(), "mute_min = 0: không tắt tiếng");
  }

  printf("\n=== alarm: tắt tiếng ===\n");
  {
    Alarm a; a.configure(true, 30);
    tk(a, 0, 2);
    a.mute(1000);
    expect(a.muted() && !tk(a, 1000, 2).on, "Tắt tiếng: im lặng");
    expect(!tk(a, 30000, 2).on, "Tắt tiếng: tại thời điểm vốn có tiếng vẫn im");
    expect(!tk(a, 1800999, 2).on && a.muted(), "Còn 1 ms: vẫn tắt tiếng");
    tk(a, 1801000, 2);
    expect(!a.muted(), "Đủ 30 phút: hết tắt tiếng");
    expect(tk(a, 1830000, 2).on, "Sau đó còi kêu lại theo chu kỳ");
  }
  {
    Alarm a; a.configure(true, 30);
    tk(a, 0, 1); a.mute(10);
    expect(!tk(a, 20, 1).on, "Đang tắt tiếng ở mức 1");
    expect(tk(a, 5000, 2).on && !a.muted(), "Risk tăng: tắt tiếng bị hủy, kêu ngay");
  }
  {
    Alarm a; a.configure(true, 30);
    tk(a, 0, 2); a.mute(10);
    tk(a, 100, 1);
    expect(a.muted(), "Risk giảm nhưng chưa về 0: vẫn tắt tiếng");
    tk(a, 200, 2);
    expect(a.muted(), "Risk quay lại đúng mức cũ: vẫn tắt tiếng");
    tk(a, 300, 0);
    expect(!a.muted(), "Risk về 0: sự cố đã qua, hủy tắt tiếng");
    expect(tk(a, 400, 2).on, "Sự cố mới sau đó: còi kêu bình thường");
  }

  printf("\n=== alarm: millis quay vòng và duty ===\n");
  {
    Alarm a;
    uint32_t t0 = 0xFFFFFF00u;
    expect(tk(a, t0, 1).on, "Mức 1 bắt đầu sát lúc millis quay vòng: kêu");
    expect(!tk(a, (uint32_t)(t0 + 200u), 1).on, "Sau 200 ms (đã quay vòng): hết tiếng");
    expect(tk(a, (uint32_t)(t0 + 60000u), 1).on, "Sau 60 giây (đã quay vòng): kêu lại");
  }
#if !BUZZ_ACTIVE_LOW
  { AlarmOut on50 = {true, 50}, on15 = {true, 15}, off = {false, 0};
    expect(alarmRawDuty(on50, 8) == 127, "Duty 50 % ở 8 bit = 127");
    expect(alarmRawDuty(on15, 8) == 38, "Duty 15 % ở 8 bit = 38");
    expect(alarmRawDuty(off, 8) == 0, "Im lặng = duty 0"); }
#endif

  printf("\nTổng: %d ca, sai %d\n", total, failures);
  if (failures == 0) printf("TẤT CẢ ĐẠT\n");
  return failures == 0 ? 0 : 1;
}
