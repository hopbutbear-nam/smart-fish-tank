# firmware/

Mã nguồn Arduino cho ESP32. Môi trường: Arduino IDE 2.x, board **ESP32 Dev Module**, Serial 115200.

- `fish_tank/`: chương trình chính của hệ thống.
  - Hằng số hiệu chuẩn nằm ở `config.h`; đã đặt `CALIBRATION_DONE = 1` (commit `e7f56f3`, 01/11/2026).
  - Gồm 6 mô-đun chức năng:
    - `sensors`: Đọc dữ liệu từ cảm biến nhiệt độ DS18B20, module độ đục, module pH và cảm biến mực nước; xử lý lọc nhiễu và phát hiện lỗi phần cứng.
    - `risk`: Đánh giá rủi ro theo ngưỡng an toàn đa chỉ số, trả về mức `risk` tổng hợp và tập cờ `flags`.
    - `alarm`: Quản lý còi cảnh báo tại chỗ 3 mức âm thanh và hỗ trợ tắt tiếng 30 phút qua lệnh `cmd/mute`.
    - `feeder`: Điều khiển servo cho ăn, kiểm soát thời gian quay, chống kẹt và khóa hạn mức cho ăn theo cửa sổ trượt 24 giờ.
    - `net`: Quản lý kết nối Wi-Fi, đồng bộ giờ NTP, kết nối MQTT TLS với EMQX Cloud, gửi telemetry/event và nhận lệnh/config.
    - `store`: Lưu trữ cấu hình ngưỡng an toàn và lịch sử các lần cho ăn vào bộ nhớ flash NVS của ESP32.
- `tests/`: các bài thử từng thiết bị (Giai đoạn 0 đến 2) và bài thử tiện ích:
  - `reset_feed_history/`: Xóa lịch sử cho ăn trong NVS, không đụng cấu hình; dùng sau khi thử nghiệm T3.7, rồi nạp lại `fish_tank`.
  - Các bài thử thiết bị: mỗi bài một thư mục, tên thư mục trùng tên file `.ino`.
- `certs/`: chứng chỉ CA của EMQX (`emqx-ca.pem`, công khai, không phải bí mật).

## Kiểm thử logic trên máy tính
Ba bộ ca thử kiểm tra logic của firmware độc lập không cần phần cứng nằm tại thư mục `tools/`:
- `tools/test-risk/test_risk.cpp` (72 ca, sai 0)
- `tools/test-sensors-alarm/test_sensors_alarm.cpp` (101 ca, sai 0)
- `tools/test-store-feeder/test_store_feeder.cpp` (89 ca, sai 0; sai 4 nếu FEED_MAX_PER_24H = 20)
Chi tiết lệnh biên dịch và kết quả mong đợi xem tại `README.md` ở thư mục gốc.

## Quy tắc an toàn và bảo mật
- Không đưa `secrets.h` lên GitHub (đã nằm trong `.gitignore`). Dùng `secrets.example.h` làm mẫu.
- Không tự đổi tên topic, số chân GPIO, định dạng JSON: sửa mục E của kế hoạch trước, rồi mới sửa code.
- Ghi phiên bản gói esp32 (Boards Manager) vào `documentation/` mỗi khi biên dịch ra kết quả quan trọng.

## Thư viện cần cài
OneWire, DallasTemperature, PubSubClient, ArduinoJson (bản 7), ESP32Servo.
