# tools/

Công cụ hỗ trợ phát triển và kiểm thử hệ thống IoT-fish-tank trên máy tính:

- `mqtt-samples/`: Chứa các bản tin MQTT mẫu JSON để thử nghiệm publish/subscribe bằng MQTTX hoặc script.
- `fake-esp32/`: Kịch bản Python giả lập hoạt động của ESP32 (gửi telemetry, nhận lệnh feed/mute/config) để kiểm thử web frontend.
- `test-risk/`: Bộ kiểm thử đơn vị C++ cho hàm đánh giá rủi ro `risk.cpp` (72 ca thử nghiệm).
- `test-sensors-alarm/`: Bộ kiểm thử đơn vị C++ cho logic xử lý cảm biến và chu kỳ còi cảnh báo (101 ca thử nghiệm).
- `test-store-feeder/`: Bộ kiểm thử đơn vị C++ cho logic bộ đếm cho ăn, giới hạn 24 giờ và lưu trữ cấu hình (89 ca thử nghiệm).

Lưu ý: Các tệp thực thi biên dịch (`*.exe`) không được commit vào kho (đã được cấu hình trong `.gitignore`).
