// test_store_feeder.cpp: thử phần THUẦN của store.cpp và feeder.cpp trên máy tính (chưa cần ESP32).
// Biên dịch từ thư mục gốc của kho (một dòng):
//   g++ -std=c++11 -Wall -I firmware/fish_tank tools/test-store-feeder/test_store_feeder.cpp firmware/fish_tank/store.cpp firmware/fish_tank/feeder.cpp firmware/fish_tank/risk.cpp -o tools/test-store-feeder/test_store_feeder.exe
// Chạy: chcp 65001   rồi   .\tools\test-store-feeder\test_store_feeder.exe
// Liên quan bài thử: T3.5, T3.6, T3.7, T3.8 (phần logic; phần servo và NVS thật thử khi có ESP32).
#include <stdio.h>
#include <string.h>
#include "store.h"
#include "feeder.h"

static int total = 0, failures = 0;

static void expect(bool cond, const char* name) {
  total++;
  if (!cond) failures++;
  printf("%s  %s\n", cond ? "PASS" : "FAIL", name);
}

static const char* D(FeedDecision d) {
  switch (d) {
    case FEED_ACCEPT: return "ACCEPT";
    case FEED_IGNORE_DUP: return "IGNORE_DUP";
    case FEED_NO_TIME: return "NO_TIME";
    case FEED_BUSY: return "BUSY";
    case FEED_TOO_SOON: return "TOO_SOON";
    case FEED_DAILY_LIMIT: return "DAILY_LIMIT";
  }
  return "?";
}

static void expectDecision(FeedDecision got, FeedDecision want, const char* name) {
  char buf[160];
  snprintf(buf, sizeof(buf), "%s  [được %s, mong đợi %s]", name, D(got), D(want));
  expect(got == want, buf);
}

// 00:00 giờ địa phương (UTC+7) của một ngày bất kỳ: (MID + TZ_OFFSET_S) chia hết cho 86400
static const uint32_t MID = 20756UL * 86400UL - (uint32_t)TZ_OFFSET_S;
static uint32_t at(int h, int m, int s = 0) { return MID + h * 3600UL + m * 60UL + s; }

int main() {
  char err[64];

  printf("=== store: kiểm tra cấu hình ===\n");
  AppConfig c = configDefaults();
  expect(configValidate(c, err, sizeof(err)), "Cấu hình mặc định hợp lệ");

  c = configDefaults(); c.temp_min = 18.0f; c.temp_max = 33.0f;
  expect(configValidate(c, err, sizeof(err)), "Đúng biên nhiệt độ 18 và 33: hợp lệ");
  c = configDefaults(); c.temp_min = 17.9f;
  expect(!configValidate(c, err, sizeof(err)), "temp_min 17,9 (ngoài biên): từ chối");
  c = configDefaults(); c.temp_max = 33.1f;
  expect(!configValidate(c, err, sizeof(err)), "temp_max 33,1 (ngoài biên): từ chối");
  c = configDefaults(); c.temp_min = 29.0f;
  expect(!configValidate(c, err, sizeof(err)), "temp_min = temp_max: từ chối");
  c = configDefaults(); c.ph_min = 5.0f; c.ph_max = 8.5f;
  expect(configValidate(c, err, sizeof(err)), "Đúng biên pH 5,0 và 8,5: hợp lệ");
  c = configDefaults(); c.ph_max = 8.6f;
  expect(!configValidate(c, err, sizeof(err)), "ph_max 8,6 (ngoài biên): từ chối");
  c = configDefaults(); c.ph_min = 4.9f;
  expect(!configValidate(c, err, sizeof(err)), "ph_min 4,9 (ngoài biên): từ chối");
  c = configDefaults(); c.level_danger_pct = 40.0f;
  expect(!configValidate(c, err, sizeof(err)), "level_danger lớn hơn level_min: từ chối");
  c = configDefaults(); c.turb_max_pct = 90.0f;
  expect(!configValidate(c, err, sizeof(err)), "turb_max 90 (+15 vượt 100): từ chối");
  c = configDefaults(); c.feed_ms = 99;
  expect(!configValidate(c, err, sizeof(err)), "feed_ms 99: từ chối");
  c = configDefaults(); c.feed_ms = 3001;
  expect(!configValidate(c, err, sizeof(err)), "feed_ms 3001: từ chối");
  c = configDefaults(); c.mute_min = 1441;
  expect(!configValidate(c, err, sizeof(err)), "mute_min 1441: từ chối");
  c = configDefaults(); c.temp_max = 0.0f / 0.0f;
  expect(!configValidate(c, err, sizeof(err)), "NaN: từ chối");

  printf("\n=== store: giờ cho ăn HH:MM ===\n");
  uint16_t mins = 0;
  expect(parseHHMM("08:00", &mins) && mins == 480, "08:00 -> 480 phút");
  expect(parseHHMM("23:59", &mins) && mins == 1439, "23:59 -> 1439 phút");
  expect(parseHHMM("00:00", &mins) && mins == 0, "00:00 -> 0 phút");
  expect(!parseHHMM("24:00", &mins), "24:00: từ chối");
  expect(!parseHHMM("08:60", &mins), "08:60: từ chối");
  expect(!parseHHMM("8:00", &mins), "8:00 (thiếu số 0): từ chối");
  expect(!parseHHMM("ab:cd", &mins), "ab:cd: từ chối");
  expect(!parseHHMM("", &mins), "chuỗi rỗng: từ chối");
  char hm[6]; formatHHMM(1080, hm);
  expect(strcmp(hm, "18:00") == 0, "formatHHMM(1080) = 18:00");

  c = configDefaults();
  { const char* t[] = {"18:00", "08:00"};
    expect(configParseFeedTimes(t, 2, c, err, sizeof(err)) && c.feed_count == 2 &&
           c.feed_min[0] == 480 && c.feed_min[1] == 1080, "Hai mốc: tự sắp tăng dần (08:00, 18:00)"); }
  { const char* t[] = {"08:00", "08:00"};
    AppConfig d = configDefaults();
    expect(!configParseFeedTimes(t, 2, d, err, sizeof(err)) && d.feed_count == 0, "Hai mốc trùng: từ chối, không đổi cấu hình"); }
  { const char* t[] = {"06:00", "12:00", "18:00", "21:00"};
    AppConfig d = configDefaults();
    expect(!configParseFeedTimes(t, 4, d, err, sizeof(err)), "4 mốc (D6 chỉ cho 3): từ chối"); }
  { const char* t[] = {"08:00", "9:30"};
    AppConfig d = configDefaults();
    expect(!configParseFeedTimes(t, 2, d, err, sizeof(err)) && d.feed_count == 0, "Một mốc sai dạng: từ chối cả danh sách"); }

  printf("\n=== feeder: tiện ích ===\n");
  expect(feedClampPortions(0) == 1 && feedClampPortions(-3) == 1, "Số phần 0 hoặc âm: kẹp về 1");
  expect(feedClampPortions(1) == 1 && feedClampPortions(2) == 2, "Số phần 1 và 2: giữ nguyên");
  expect(feedClampPortions(5) == MAX_PORTIONS, "Số phần 5: kẹp về 2");
  expect(feedDurationMs(600, 2) == 1200, "600 ms x 2 phần = 1200 ms");
  expect(feedDurationMs(3000, 2) == 6000, "3000 ms x 2 phần = 6000 ms");
  expect(feedDurationMs(6000, 2) == FEED_HARD_CAP_MS, "Vượt mốc cứng: cắt ở 10 giây");
  expect(!feedTimeValid(0) && feedTimeValid(at(8, 0)), "Epoch 0 = chưa có giờ; epoch thật = có giờ");

  printf("\n=== feeder: lệnh từ xa (T3.5, T3.6, T3.7) ===\n");
  {
    FeedPolicy p;
    uint32_t t = at(9, 0);
    expectDecision(p.decideRemote("a1", 0, false), FEED_NO_TIME, "Chưa có giờ NTP");
    expectDecision(p.decideRemote("a2", t, false), FEED_ACCEPT, "Lệnh đầu tiên");
    p.commit(t);
    expectDecision(p.decideRemote("a2", t + 1, false), FEED_IGNORE_DUP, "Gửi lại cùng id: bỏ qua");
    expectDecision(p.decideRemote("a3", t + 5, false), FEED_TOO_SOON, "Lệnh mới ngay sau đó: too_soon");
    expectDecision(p.decideRemote("a4", t + 5, true), FEED_BUSY, "Đang quay: busy (ưu tiên hơn too_soon)");
    expectDecision(p.decideRemote("a5", t + 59, false), FEED_TOO_SOON, "Sau 59 giây: vẫn too_soon");
    expectDecision(p.decideRemote("a6", t + 60, false), FEED_ACCEPT, "Đúng 60 giây: được");
    p.commit(t + 60);
    expectDecision(p.decideRemote("", t + 130, false), FEED_ACCEPT, "Id rỗng: không kiểm tra trùng");
    p.commit(t + 130);
    expectDecision(p.decideRemote("a7", t + 200, false), FEED_ACCEPT, "Lần thứ 4");
    p.commit(t + 200);
    expect(p.count() == 4, "Đã lưu 4 mốc");
    expectDecision(p.decideRemote("a8", t + 300, false), FEED_DAILY_LIMIT, "Lần thứ 5 trong 24 giờ: daily_limit");
    expectDecision(p.decideRemote("a9", t + 86399, false), FEED_DAILY_LIMIT, "Mốc cũ nhất chưa đủ 24 giờ: vẫn daily_limit");
    expectDecision(p.decideRemote("b1", t + 86400, false), FEED_ACCEPT, "Mốc cũ nhất đủ 24 giờ: cửa sổ trượt cho phép");
  }
  {
    // nạp lịch sử từ NVS rồi quyết định (T3.8: còn giữ sau khởi động lại)
    FeedPolicy p;
    uint32_t t = at(9, 0);
    uint32_t saved[4] = {t, t + 61, t + 122, t + 183};
    p.load(saved, 4);
    expectDecision(p.decideRemote("c1", t + 300, false), FEED_DAILY_LIMIT, "Sau khi nạp từ NVS: hạn mức vẫn còn");
  }
  {
    FeedPolicy p;
    uint32_t t = at(9, 0);
    p.commit(t);
    expectDecision(p.decideScheduled(t + 10, false), FEED_TOO_SOON, "Lịch cũng chịu too_soon");
    expectDecision(p.decideScheduled(t + 70, false), FEED_ACCEPT, "Lịch được phép sau 60 giây");
  }

  printf("\n=== feeder: lịch tự động (D6) ===\n");
  {
    uint16_t slots[2] = {480, 1080};      // 08:00 và 18:00
    FeedPolicy p;
    expect(p.scheduleDue(slots, 2, at(7, 59, 59)) == -1, "07:59:59: chưa tới giờ");
    expect(p.scheduleDue(slots, 2, at(8, 0, 0)) == 0, "08:00:00: mốc 0 đến giờ");
    expect(p.scheduleDue(slots, 2, at(8, 4, 59)) == 0, "08:04:59: vẫn trong cửa sổ 5 phút");
    expect(p.scheduleDue(slots, 2, at(8, 5, 0)) == -1, "08:05:00: lỡ cửa sổ, bỏ qua (không cho ăn bù)");
    expect(p.scheduleDue(slots, 2, at(18, 1)) == 1, "18:01: mốc 1 đến giờ");
    expect(p.scheduleDue(slots, 2, 0) == -1, "Chưa có giờ NTP: không chạy lịch");
    p.commit(at(8, 0, 10));
    expect(p.scheduleDue(slots, 2, at(8, 2)) == -1, "Đã cho ăn từ 08:00: không lặp trong cùng cửa sổ (kể cả sau khởi động lại)");
    expect(p.scheduleDue(slots, 2, at(18, 1)) == 1, "Mốc 18:00 vẫn chạy bình thường");
  }
  {
    uint16_t slots[1] = {480};
    FeedPolicy p;
    p.commit(at(7, 58));                   // cho ăn tay trước giờ đặt
    expect(p.scheduleDue(slots, 1, at(8, 0)) == 0, "Cho ăn tay lúc 07:58 không chặn mốc 08:00");
  }
  {
    uint16_t slots[1] = {23 * 60 + 58};   // 23:58
    FeedPolicy p;
    expect(p.scheduleDue(slots, 1, at(23, 59)) == 0, "23:58: đến giờ lúc 23:59");
    expect(p.scheduleDue(slots, 1, MID + 86400UL + 60UL) == 0, "Cửa sổ vắt qua nửa đêm: 00:01 hôm sau vẫn đến giờ");
    expect(p.scheduleDue(slots, 1, MID + 86400UL + 4UL * 60UL) == -1, "00:04 hôm sau: hết cửa sổ");
  }
  {
    // lịch bị hạn mức chặn: 4 lần trong 24 giờ
    FeedPolicy p;
    uint32_t t = at(1, 0);
    p.commit(t); p.commit(t + 100); p.commit(t + 200); p.commit(t + 300);
    expectDecision(p.decideScheduled(at(8, 0), false), FEED_DAILY_LIMIT, "Lịch 08:00 khi đã đủ 4 lần: daily_limit");
  }

  printf("\n=== feeder: máy trạng thái quay servo ===\n");
  {
    FeedRunner r;
    expect(!r.busy() && r.servoUs() == SERVO_STOP_US, "Ban đầu: không bận, servo ở điểm dừng");
    expect(r.start(1000, 600, "id1", 1), "Bắt đầu quay 600 ms");
    expect(r.busy() && r.servoUs() == SERVO_FEED_US, "Đang quay: servo ở xung quay");
    expect(!r.start(1100, 600, "id2", 1), "Đang bận: không nhận lệnh thứ hai");
    expect(!r.tick(1599), "Tại 599 ms: chưa xong");
    expect(r.tick(1600), "Tại 600 ms: xong (báo đúng một lần)");
    expect(!r.tick(1700), "Gọi tick lần nữa: không báo lại");
    expect(!r.busy() && r.servoUs() == SERVO_STOP_US, "Xong: servo về điểm dừng");
    expect(strcmp(r.id(), "id1") == 0 && r.portions() == 1, "id và số phần của lần vừa xong");
  }
  {
    FeedRunner r;
    r.start(0, 99999, "x", 2);
    expect(!r.tick(FEED_HARD_CAP_MS - 1), "Thời gian quá lớn bị cắt: chưa xong ở 9,999 giây");
    expect(r.tick(FEED_HARD_CAP_MS), "Mốc cứng 10 giây: luôn dừng");
  }
  {
    FeedRunner r;
    const uint32_t nearWrap = 0xFFFFFF00u;                  // millis() sắp quay vòng về 0
    r.start(nearWrap, 600, "w", 1);
    expect(!r.tick((uint32_t)(nearWrap + 599u)), "millis quay vòng: chưa xong ở 599 ms");
    expect(r.tick((uint32_t)(nearWrap + 600u)), "millis quay vòng: vẫn đúng ở 600 ms");
  }

  printf("\nTổng: %d ca, sai %d\n", total, failures);
  if (failures == 0) printf("TẤT CẢ ĐẠT\n");
  return failures == 0 ? 0 : 1;
}
