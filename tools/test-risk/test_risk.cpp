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
  in.turb_ok = true;  in.turb_pct = 12.0f;
  in.level_ok = true; in.level_pct = 80.0f;
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

static void expectTrue(bool ok, const char* name) {
  total++;
  if (!ok) failures++;
  printf("%s  %s\n", ok ? "PASS" : "FAIL", name);
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
  in = normalInput(); in.turb_ok = false;                check("Cảm biến độ đục lỗi", riskEvaluate(in, th), 1, "sensor_turb");
  in = normalInput(); in.level_ok = false;               check("Cảm biến mực nước lỗi", riskEvaluate(in, th), 1, "sensor_level");
  in = normalInput(); in.temp_ok = false; in.ph_ok = false; in.turb_ok = false; in.level_ok = false;
                                                         check("4 cảm biến cùng lỗi (4 cờ mức 1)", riskEvaluate(in, th), 3, "sensor_temp,sensor_ph,sensor_turb,sensor_level");
  in = normalInput(); in.turb_ok = false; in.turb_pct = 99.0f;
                                                         check("Độ đục lỗi: bỏ qua số đo, không báo turb_high", riskEvaluate(in, th), 1, "sensor_turb");
  in = normalInput(); in.level_ok = false; in.level_pct = 0.0f;
                                                         check("Mực nước lỗi: bỏ qua số đo, không báo level_low", riskEvaluate(in, th), 1, "sensor_level");
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
    RiskInput near = normalInput(); near.temp_c = 28.7f;      // đã dưới 29 nhưng chưa tới 28,5 (29 - 0,5)
    check("Về 28,7 (vùng giữa): giữ mức 1", f.update(near, th), 1, "temp_high");
    RiskInput edge = normalInput(); edge.temp_c = 28.5f;      // đúng 28,5: hạ (so sánh nghiêm ngặt)
    check("Về đúng 28,5 (29 - 0,5): hạ về 0", f.update(edge, th), 0, "");
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

  // ---- Khoảng đệm khi hạ mức: 0,5 °C, 0,3 pH, 5 điểm % (nhóm chốt, xem documentation/fix-plan.md) ----
  printf("\n=== Quy tắc hạ mức (khoảng đệm 0,5 / 0,3 / 5) ===\n");
  {
    RiskFilter f; RiskInput x = normalInput();
    x.temp_c = 30.0f; f.update(x, th); f.update(x, th); f.update(x, th);
    x.temp_c = 28.6f; check("Nhiệt độ cao: về 28,6 (chưa tới 28,5) giữ mức 1", f.update(x, th), 1, "temp_high");
    x.temp_c = 28.5f; check("Nhiệt độ cao: về 28,5 hạ về 0", f.update(x, th), 0, "");
  }
  {
    RiskFilter f; RiskInput x = normalInput();
    x.temp_c = 22.0f; f.update(x, th); f.update(x, th); f.update(x, th);
    x.temp_c = 23.4f; check("Nhiệt độ thấp: về 23,4 (chưa tới 23,5) giữ mức 1", f.update(x, th), 1, "temp_low");
    x.temp_c = 23.5f; check("Nhiệt độ thấp: về 23,5 hạ về 0", f.update(x, th), 0, "");
  }
  {
    RiskFilter f; RiskInput x = normalInput();
    x.ph = 8.2f; f.update(x, th); f.update(x, th); f.update(x, th);
    x.ph = 7.8f; check("pH cao: về 7,8 (chưa tới 7,7) giữ mức 1", f.update(x, th), 1, "ph_high");
    x.ph = 7.7f; check("pH cao: về 7,7 hạ về 0", f.update(x, th), 0, "");
  }
  {
    RiskFilter f; RiskInput x = normalInput();
    x.turb_pct = 35.0f; f.update(x, th); f.update(x, th); f.update(x, th);
    x.turb_pct = 26.0f; check("Độ đục: về 26 (chưa tới 25) giữ mức 1", f.update(x, th), 1, "turb_high");
    x.turb_pct = 25.0f; check("Độ đục: về 25 hạ về 0", f.update(x, th), 0, "");
  }
  {
    RiskFilter f; RiskInput x = normalInput();
    x.level_pct = 25.0f; f.update(x, th); f.update(x, th); f.update(x, th);
    x.level_pct = 34.0f; check("Mực nước: lên 34 (chưa tới 35) giữ mức 1", f.update(x, th), 1, "level_low");
    x.level_pct = 35.0f; check("Mực nước: lên 35 hạ về 0", f.update(x, th), 0, "");
  }
  {
    RiskFilter f; RiskInput x = normalInput();
    x.level_pct = 10.0f; f.update(x, th); f.update(x, th); f.update(x, th);
    x.level_pct = 19.0f; check("Mực nước mức 2: lên 19 (chưa tới 20) giữ mức 2", f.update(x, th), 2, "level_low");
    x.level_pct = 20.0f; check("Mực nước mức 2: lên 20 hạ về mức 1", f.update(x, th), 1, "level_low");
  }

  // ---- #9 hướng A: khoảng đệm cho biên nguy hiểm ----
  printf("\n=== Khoảng đệm ở biên nguy hiểm (#9) ===\n");
  {
    RiskFilter f; RiskInput x = normalInput();
    x.temp_c = 33.1f; f.update(x, th); f.update(x, th);
    check("Nhiệt độ 33,1 lần 3: nâng lên mức 3", f.update(x, th), 3, "temp_high");
    x.temp_c = 32.98f; check("Về 32,98: vẫn mức 3 (không nhấp nháy)", f.update(x, th), 3, "temp_high");
    x.temp_c = 32.6f;  check("Về 32,6: vẫn mức 3", f.update(x, th), 3, "temp_high");
    x.temp_c = 32.5f;  check("Về đúng 32,5 (33 - 0,5): hạ xuống mức 2", f.update(x, th), 2, "temp_high");
  }
  {
    RiskFilter f; RiskInput x = normalInput();
    x.temp_c = 17.9f; f.update(x, th); f.update(x, th);
    check("Nhiệt độ 17,9 lần 3: nâng lên mức 3", f.update(x, th), 3, "temp_low");
    x.temp_c = 18.4f; check("Lên 18,4: vẫn mức 3", f.update(x, th), 3, "temp_low");
    x.temp_c = 18.5f; check("Lên đúng 18,5 (18 + 0,5): hạ xuống mức 2", f.update(x, th), 2, "temp_low");
  }
  {
    RiskFilter f; RiskInput x = normalInput();
    x.ph = 8.6f; f.update(x, th); f.update(x, th);
    check("pH 8,6 lần 3: nâng lên mức 2", f.update(x, th), 2, "ph_high");
    x.ph = 8.3f; check("Về 8,3: vẫn mức 2", f.update(x, th), 2, "ph_high");
    x.ph = 8.2f; check("Về đúng 8,2 (8,5 - 0,3): hạ xuống mức 1", f.update(x, th), 1, "ph_high");
  }
  {
    RiskFilter f; RiskInput x = normalInput();
    x.ph = 4.9f; f.update(x, th); f.update(x, th);
    check("pH 4,9 lần 3: nâng lên mức 2", f.update(x, th), 2, "ph_low");
    x.ph = 5.2f; check("Lên 5,2: vẫn mức 2", f.update(x, th), 2, "ph_low");
    x.ph = 5.3f; check("Lên đúng 5,3 (5,0 + 0,3): hạ xuống mức 1", f.update(x, th), 1, "ph_low");
  }

  // ---- Cờ lỗi cảm biến mới đi qua bộ lọc (cần 3 lần liên tiếp) ----
  printf("\n=== Lỗi cảm biến độ đục và mực nước qua bộ lọc ===\n");
  {
    RiskFilter f; RiskInput bad = normalInput(); bad.turb_ok = false; bad.level_ok = false;
    f.update(bad, th); f.update(bad, th);
    check("Hai cảm biến lỗi 2 lần: chưa đặt cờ", f.update(normalInput(), th), 0, "");
    f.update(bad, th); f.update(bad, th);
    check("Lỗi 3 lần liên tiếp: đặt sensor_turb và sensor_level", f.update(bad, th), 1, "sensor_turb,sensor_level");
    check("Hết lỗi: bỏ cờ", f.update(normalInput(), th), 0, "");
  }

  // ---- Bảo vệ khi ngưỡng hẹp bất thường: khoảng đệm không làm ngưỡng thu hẹp chồng nhau ----
  printf("\n=== riskTighten: biên và bảo vệ ===\n");
  {
    RiskThresholds d = riskDefaultThresholds();
    expectTrue(d.temp_danger_min == 18.0f && d.temp_danger_max == 33.0f && d.ph_danger_min == 5.0f && d.ph_danger_max == 8.5f,
               "riskDefaultThresholds có đủ 4 biên nguy hiểm");
    RiskThresholds t = riskTighten(d);
    expectTrue(t.temp_danger_min == 18.5f && t.temp_danger_max == 32.5f && t.temp_min == 23.5f && t.temp_max == 28.5f,
               "riskTighten mặc định: nhiệt độ 23,5 đến 28,5; biên nguy hiểm 18,5 đến 32,5");
    expectTrue(t.turb_max_pct == 25.0f && t.level_min_pct == 35.0f && t.level_danger_pct == 20.0f,
               "riskTighten mặc định: độ đục 25; mực nước 35 và 20");
    RiskThresholds nar = d; nar.temp_min = 26.0f; nar.temp_max = 27.0f; nar.turb_max_pct = 4.0f; nar.level_min_pct = 98.0f; nar.level_danger_pct = 97.0f;
    RiskThresholds tn = riskTighten(nar);
    expectTrue(tn.temp_min < tn.temp_max && tn.turb_max_pct > 0.0f && tn.level_min_pct <= 100.0f && tn.level_danger_pct <= tn.level_min_pct,
               "Ngưỡng hẹp: ngưỡng thu hẹp không chồng nhau, độ đục > 0, mực nước <= 100");
  }

  printf("\nTổng: %d ca, sai %d\n", total, failures);
  if (failures == 0) printf("TẤT CẢ ĐẠT\n");
  return failures == 0 ? 0 : 1;
}
