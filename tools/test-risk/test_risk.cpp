// test_risk.cpp: chạy bảng ca thử risk (mục E8) trên hàm tính risk của firmware, ngay trên máy tính (T3.11).
// Biên dịch từ thư mục gốc của kho:
//   g++ -std=c++11 -Wall -I firmware/fish_tank tools/test-risk/test_risk.cpp firmware/fish_tank/risk.cpp -o test_risk
//   ./test_risk        (Windows: test_risk.exe)
// Kết quả: in PASS/FAIL từng ca; thoát với mã khác 0 nếu có ca sai.
// QUY TẮC: ca nào sai thì sửa risk.cpp theo bảng E8, KHÔNG sửa bảng cho khớp code.
#include <stdio.h>
#include <string.h>
#include "risk.h"
#include "config.h"

static int failures = 0, total = 0;

static RiskInput normalInput() {
  RiskInput in;
  in.temp_ok = true;  in.temp_c = 26.0f;
  in.ph_ok = true;    in.ph = 7.0f;
  in.turb_pct = 12.0f;
  in.level_pct = 80.0f;
  return in;
}

static void check(const char* name, const RiskResult& r, int wantRisk, const char* wantFlags) {
  char flags[96];
  riskFlagsCsv(r.levels, flags, sizeof(flags));
  bool ok = (r.risk == wantRisk) && strcmp(flags, wantFlags) == 0;
  total++;
  if (!ok) failures++;
  printf("%s  %-58s risk=%d flags=[%s]%s\n", ok ? "PASS" : "FAIL", name, r.risk, flags,
         ok ? "" : "   <-- đúng ra: risk, cờ ở cột dưới");
  if (!ok) printf("      mong đợi risk=%d flags=[%s]\n", wantRisk, wantFlags);
}

int main() {
  const RiskThresholds th = riskDefaultThresholds();
  RiskInput in;

  printf("=== Bảng ca thử E8 (hàm thuần riskEvaluate) ===\n");
  in = normalInput();                                    check("Mọi chỉ số bình thường", riskEvaluate(in, th), 0, "");
  in = normalInput(); in.temp_c = 30.0f;                 check("Nhiệt độ 30", riskEvaluate(in, th), 1, "temp_high");
  in = normalInput(); in.temp_c = 22.0f;                 check("Nhiệt độ 22", riskEvaluate(in, th), 1, "temp_low");
  in = normalInput(); in.temp_c = 32.0f;                 check("Nhiệt độ 32", riskEvaluate(in, th), 2, "temp_high");
  in = normalInput(); in.temp_c = 35.0f;                 check("Nhiệt độ 35", riskEvaluate(in, th), 3, "temp_high");
  in = normalInput(); in.temp_c = 17.5f;                 check("Nhiệt độ 17,5", riskEvaluate(in, th), 3, "temp_low");
  in = normalInput(); in.temp_c = 29.0f;                 check("Nhiệt độ đúng 29,0 (so sánh nghiêm ngặt)", riskEvaluate(in, th), 0, "");
  in = normalInput(); in.ph = 5.5f;                      check("pH 5,5", riskEvaluate(in, th), 1, "ph_low");
  in = normalInput(); in.ph = 8.2f;                      check("pH 8,2", riskEvaluate(in, th), 1, "ph_high");
  in = normalInput(); in.ph = 8.5f;                      check("pH đúng 8,5 (chưa vượt biên)", riskEvaluate(in, th), 1, "ph_high");
  in = normalInput(); in.ph = 8.7f;                      check("pH 8,7", riskEvaluate(in, th), 2, "ph_high");
  in = normalInput(); in.ph = 4.5f;                      check("pH 4,5", riskEvaluate(in, th), 2, "ph_low");
  in = normalInput(); in.ph = 3.0f;                      check("pH 3,0 (pH không bao giờ ra mức 3)", riskEvaluate(in, th), 2, "ph_low");
  in = normalInput(); in.turb_pct = 35.0f;               check("Độ đục 35", riskEvaluate(in, th), 1, "turb_high");
  in = normalInput(); in.turb_pct = 50.0f;               check("Độ đục 50", riskEvaluate(in, th), 2, "turb_high");
  in = normalInput(); in.level_pct = 25.0f;              check("Mực nước 25", riskEvaluate(in, th), 1, "level_low");
  in = normalInput(); in.level_pct = 10.0f;              check("Mực nước 10", riskEvaluate(in, th), 2, "level_low");
  in = normalInput(); in.temp_ok = false;                check("Cảm biến nhiệt độ lỗi", riskEvaluate(in, th), 1, "sensor_temp");
  in = normalInput(); in.ph_ok = false;                  check("Cảm biến pH lỗi", riskEvaluate(in, th), 1, "sensor_ph");
  in = normalInput(); in.temp_c = 30; in.turb_pct = 35; in.level_pct = 25;
                                                         check("3 cờ mức 1 (30 °C, 35%, 25%)", riskEvaluate(in, th), 2, "temp_high,turb_high,level_low");
  in.ph = 8.2f;                                          check("4 cờ mức 1 (thêm pH 8,2)", riskEvaluate(in, th), 3, "temp_high,ph_high,turb_high,level_low");
  in = normalInput(); in.temp_c = 32; in.turb_pct = 50;  check("2 cờ mức 2 (32 °C, độ đục 50)", riskEvaluate(in, th), 3, "temp_high,turb_high");
  in = normalInput(); in.temp_c = 32; in.ph = 8.2f; in.turb_pct = 35; in.level_pct = 25;
                                                         check("1 cờ mức 2 và 3 cờ mức 1 (luật không đảo ngược)", riskEvaluate(in, th), 3, "temp_high,ph_high,turb_high,level_low");
  in = normalInput(); in.ph = 8.7f; in.turb_pct = 50;    check("pH 8,7 và độ đục 50 (2 cờ mức 2)", riskEvaluate(in, th), 3, "ph_high,turb_high");

  printf("\n=== Bộ lọc chống nhấp nháy RiskFilter (D3) ===\n");
  {
    RiskFilter f;
    RiskInput hot = normalInput(); hot.temp_c = 30.0f;
    RiskInput ok  = normalInput();
    check("Lần 1 nhiệt độ 30: chưa nâng", f.update(hot, th), 0, "");
    check("Lần 2 nhiệt độ 30: chưa nâng", f.update(hot, th), 0, "");
    check("Lần 3 nhiệt độ 30: nâng lên mức 1", f.update(hot, th), 1, "temp_high");
    RiskInput near = normalInput(); near.temp_c = 28.9f;      // đã dưới 29 nhưng chưa dưới 28,8
    check("Về 28,9 (vùng giữa): giữ mức 1", f.update(near, th), 1, "temp_high");
    RiskInput deep = normalInput(); deep.temp_c = 28.7f;      // dưới 29 - 0,2
    check("Về 28,7 (sâu trong ngưỡng): hạ về 0", f.update(deep, th), 0, "");
    (void)ok;
  }
  {
    RiskFilter f;
    RiskInput hot = normalInput(); hot.temp_c = 30.0f;
    RiskInput ok  = normalInput();
    f.update(hot, th); f.update(hot, th);
    check("Hai lần 30 rồi một lần 26: bộ đếm về 0", f.update(ok, th), 0, "");
    f.update(hot, th); f.update(hot, th);
    check("Sau đó lại cần đủ 3 lần liên tiếp", f.update(hot, th), 1, "temp_high");
  }
  {
    RiskFilter f;
    RiskInput bad = normalInput(); bad.temp_ok = false;
    f.update(bad, th); f.update(bad, th);
    check("Lỗi cảm biến 2 lần: chưa đặt cờ", f.update(normalInput(), th), 0, "");
    f.update(bad, th); f.update(bad, th);
    check("Lỗi cảm biến 3 lần liên tiếp: đặt sensor_temp", f.update(bad, th), 1, "sensor_temp");
    check("Cảm biến hết lỗi: bỏ cờ", f.update(normalInput(), th), 0, "");
  }
  {
    RiskFilter f;
    RiskInput a = normalInput(); a.temp_c = 35.0f;            // mức 3
    RiskInput b = normalInput(); b.temp_c = 32.0f;            // mức 2
    f.update(a, th); f.update(b, th);
    check("Chuỗi mức 3, 2, 3: nâng lên mức thấp nhất (2)", f.update(a, th), 2, "temp_high");
  }

  printf("\nTổng: %d ca, sai %d\n", total, failures);
  if (failures == 0) printf("TẤT CẢ ĐẠT\n");
  return failures == 0 ? 0 : 1;
}
