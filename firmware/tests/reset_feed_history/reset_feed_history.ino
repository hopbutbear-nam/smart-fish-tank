// reset_feed_history.ino: xóa lịch sử cho ăn trong NVS, KHÔNG đụng tới cấu hình
#include <Preferences.h>
Preferences prefs;

void setup() {
  Serial.begin(115200);
  delay(500);
  prefs.begin("fishtank", false);     // đúng namespace của store.cpp
  prefs.remove("fh");                 // danh sách mốc cho ăn
  prefs.remove("fh_n");               // số lượng mốc
  Serial.printf("Da xoa lich su cho an. fh_n = %u\n", prefs.getUChar("fh_n", 0));
  prefs.end();
  Serial.println("Xong. Hay nap lai firmware chinh (fish_tank).");
}

void loop() {}