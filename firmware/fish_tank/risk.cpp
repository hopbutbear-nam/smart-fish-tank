// risk.cpp: tính risk (0 đến 3) và flags theo ngưỡng (mục E8 của kế hoạch)
// Bảng ca thử trong E8 là nguồn sự thật: sai thì sửa hàm này, không sửa bảng cho khớp code.
#include "risk.h"
#include "config.h"
#include <string.h>

static const char* const FLAG_NAMES[FL_COUNT] = {
  "temp_high", "temp_low", "ph_high", "ph_low",
  "turb_high", "level_low", "sensor_temp", "sensor_ph"
};

const char* riskFlagName(uint8_t flag) {
  return flag < FL_COUNT ? FLAG_NAMES[flag] : "?";
}

RiskThresholds riskDefaultThresholds() {
  RiskThresholds t;
  t.temp_min = DEF_TEMP_MIN;
  t.temp_max = DEF_TEMP_MAX;
  t.ph_min = DEF_PH_MIN;
  t.ph_max = DEF_PH_MAX;
  t.turb_max_pct = DEF_TURB_MAX_PCT;
  t.level_min_pct = DEF_LEVEL_MIN_PCT;
  t.level_danger_pct = DEF_LEVEL_DANGER_PCT;
  return t;
}

// Ngưỡng "chặt hơn" một khoảng nhỏ: giá trị phải nằm sâu trong vùng an toàn thì mới được hạ mức.
RiskThresholds riskTighten(const RiskThresholds& th) {
  RiskThresholds t = th;
  t.temp_min += HYST_TEMP;
  t.temp_max -= HYST_TEMP;
  t.ph_min += HYST_PH;
  t.ph_max -= HYST_PH;
  t.turb_max_pct -= HYST_PCT;
  t.level_min_pct += HYST_PCT;
  t.level_danger_pct += HYST_PCT;
  return t;
}

// Mọi phép so sánh đều NGHIÊM NGẶT (vượt quá mới tính): đúng 29,0 °C vẫn là mức 0.
void riskComputeLevels(const RiskInput& in, const RiskThresholds& th, RiskLevels& out) {
  memset(out.lv, 0, sizeof(out.lv));

  // Nhiệt độ: mức 1 ngoài [min, max]; mức 2 ngoài [min-2, max+2]; mức 3 ngoài biên nguy hiểm 18 đến 33
  if (!in.temp_ok) {
    out.lv[FL_SENSOR_TEMP] = SENSOR_FAULT_LEVEL;
  } else {
    float t = in.temp_c;
    uint8_t lv = 0;
    if (t < TEMP_DANGER_MIN || t > TEMP_DANGER_MAX) lv = 3;
    else if (t < th.temp_min - TEMP_L2_EXTRA || t > th.temp_max + TEMP_L2_EXTRA) lv = 2;
    else if (t < th.temp_min || t > th.temp_max) lv = 1;
    if (lv) out.lv[t < th.temp_min ? FL_TEMP_LOW : FL_TEMP_HIGH] = lv;
  }

  // pH: mức 1 ngoài [min, max]; mức 2 ngoài biên nguy hiểm 5,0 đến 8,5; KHÔNG có mức 3
  if (!in.ph_ok) {
    out.lv[FL_SENSOR_PH] = SENSOR_FAULT_LEVEL;
  } else {
    float p = in.ph;
    uint8_t lv = 0;
    if (p < PH_DANGER_MIN || p > PH_DANGER_MAX) lv = 2;
    else if (p < th.ph_min || p > th.ph_max) lv = 1;
    if (lv) out.lv[p < th.ph_min ? FL_PH_LOW : FL_PH_HIGH] = lv;
  }

  // Độ đục: mức 1 trên ngưỡng; mức 2 trên ngưỡng + 15 điểm %
  if (in.turb_pct > th.turb_max_pct + TURB_L2_EXTRA) out.lv[FL_TURB_HIGH] = 2;
  else if (in.turb_pct > th.turb_max_pct) out.lv[FL_TURB_HIGH] = 1;

  // Mực nước: mức 2 dưới level_danger; mức 1 dưới level_min
  if (in.level_pct < th.level_danger_pct) out.lv[FL_LEVEL_LOW] = 2;
  else if (in.level_pct < th.level_min_pct) out.lv[FL_LEVEL_LOW] = 1;
}

// Luật tổng hợp: lấy mức riêng cao nhất; từ 3 cờ (mức >= 1) thì ít nhất mức 2;
// từ 4 cờ (mức >= 1) hoặc từ 2 cờ mức >= 2 thì mức 3.
// Đếm theo "từ mức n trở lên" để tình trạng nặng hơn không bao giờ báo nhẹ hơn.
uint8_t riskAggregate(const RiskLevels& lv) {
  uint8_t risk = 0;
  uint8_t c1 = 0, c2 = 0;
  for (uint8_t i = 0; i < FL_COUNT; i++) {
    if (lv.lv[i] > risk) risk = lv.lv[i];
    if (lv.lv[i] >= 1) c1++;
    if (lv.lv[i] >= 2) c2++;
  }
  if (c1 >= 3 && risk < 2) risk = 2;
  if (c1 >= 4 || c2 >= 2) risk = 3;
  return risk;
}

RiskResult riskEvaluate(const RiskInput& in, const RiskThresholds& th) {
  RiskResult r;
  riskComputeLevels(in, th, r.levels);
  r.risk = riskAggregate(r.levels);
  return r;
}

size_t riskFlagsCsv(const RiskLevels& lv, char* buf, size_t len) {
  if (len == 0) return 0;
  size_t n = 0;
  buf[0] = '\0';
  for (uint8_t i = 0; i < FL_COUNT; i++) {
    if (lv.lv[i] == 0) continue;
    const char* name = FLAG_NAMES[i];
    size_t need = strlen(name) + (n ? 1 : 0);
    if (n + need + 1 > len) break;       // hết chỗ: dừng, không cắt dở một tên cờ
    if (n) buf[n++] = ',';
    memcpy(buf + n, name, strlen(name));
    n += strlen(name);
    buf[n] = '\0';
  }
  return n;
}

// ---------------------------------------------------------------------------
// Chống nhấp nháy (D3), áp dụng riêng cho từng cờ
//  - Nâng: mức mới phải đúng RISK_RAISE_READS lần đánh giá liên tiếp; nâng lên mức thấp nhất trong các lần đó.
//  - Hạ: chỉ khi giá trị đã quay lại sâu trong ngưỡng (ngưỡng thu hẹp một khoảng nhỏ).
//  - Vùng giữa (đã hết vượt ngưỡng nhưng chưa vào sâu): giữ nguyên mức cũ.
// Cờ lỗi cảm biến cũng đi qua bộ lọc này nên tự có điều kiện "lỗi 3 lần liên tiếp mới đặt cờ".
// ---------------------------------------------------------------------------
void RiskFilter::reset() {
  memset(cur_, 0, sizeof(cur_));
  memset(upCnt_, 0, sizeof(upCnt_));
  memset(upMin_, 0, sizeof(upMin_));
}

RiskResult RiskFilter::update(const RiskInput& in, const RiskThresholds& th) {
  RiskLevels now, strict;
  riskComputeLevels(in, th, now);
  riskComputeLevels(in, riskTighten(th), strict);

  for (uint8_t i = 0; i < FL_COUNT; i++) {
    if (now.lv[i] > cur_[i]) {
      if (upCnt_[i] == 0 || now.lv[i] < upMin_[i]) upMin_[i] = now.lv[i];
      if (upCnt_[i] < 255) upCnt_[i]++;
      if (upCnt_[i] >= RISK_RAISE_READS) {
        cur_[i] = upMin_[i];
        upCnt_[i] = 0;
      }
    } else {
      upCnt_[i] = 0;
      if (strict.lv[i] < cur_[i]) cur_[i] = strict.lv[i];
    }
  }

  RiskResult r;
  memcpy(r.levels.lv, cur_, sizeof(cur_));
  r.risk = riskAggregate(r.levels);
  return r;
}
