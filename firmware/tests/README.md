# firmware/tests/

Các bài thử từng thiết bị (Giai đoạn 0 đến 2) và tiện ích quản lý bộ nhớ:

- `test1_blink/`: Bài thử T0.1 – Nạp chương trình nhấp nháy đèn LED để kiểm tra kết nối với ESP32. (Tệp .ino nằm trên máy thành viên, chưa đưa lên kho).
- `test4_emqx/`: Bài thử T1.6 – Kiểm tra ESP32 kết nối tới EMQX Cloud qua MQTT TLS. (Tệp .ino nằm trên máy thành viên, chưa đưa lên kho).
- `test_ds18b20/`: Bài thử T2.1 – Đọc cảm biến nhiệt độ DS18B20 chống nước. (Tệp .ino nằm trên máy thành viên, chưa đưa lên kho).
- `test_level/`: Bài thử T2.2 – Đọc cảm biến mực nước analog. (Tệp .ino nằm trên máy thành viên, chưa đưa lên kho).
- `test_analog/`: Bài thử T2.3, T2.4 – Đọc tín hiệu module độ đục và pH qua cầu chia áp. (Tệp .ino nằm trên máy thành viên, chưa đưa lên kho).
- `test_servo/`: Bài thử T2.5, T2.6 – Kiểm tra servo 360°, tìm điểm dừng SERVO_STOP_US và xác định FEED_MS. (Tệp .ino nằm trên máy thành viên, chưa đưa lên kho).
- `test_buzzer/`: Bài thử T2.7 – Kiểm tra còi báo thụ động ở tần số 1300 Hz. (Tệp .ino nằm trên máy thành viên, chưa đưa lên kho).
- `reset_feed_history/`: `reset_feed_history.ino` – Xóa lịch sử cho ăn trong NVS, không đụng cấu hình; dùng sau khi thử T3.7, rồi nạp lại fish_tank. (Đã có tệp .ino trong kho).
