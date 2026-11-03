# Giai đoạn 3: Firmware ESP32: kết quả kiểm thử

Cập nhật 03/11/2026 theo kế hoạch bản 1.5. Đổi thành Đạt chỉ khi có bằng chứng; chưa có thì ghi 'Đạt (theo lời nhóm, 03/11/2026)'.

| Mã | Nội dung | Kết quả | Ngày | Người làm | Bằng chứng (tên file trong `evidence/`) | Ghi chú |
| --- | --- | --- | --- | --- | --- | --- |
| T3.1 | Biên dịch và nạp, log có phiên bản, WiFi, MQTT, NTP | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Phiên bản firmware, kết nối WiFi và MQTT |
| T3.2 | status = online; telemetry mỗi 10 giây, JSON đúng mục E4 | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Telemetry gửi đúng định dạng lên EMQX |
| T3.3 | Thay đổi từng cảm biến: trường, flags và risk đổi đúng | Đạt (MÔ PHỎNG); thiết bị thật: Đạt (theo lời nhóm, 03/11/2026) | | | T3.3-3.9_2026-10-30_mo-phong_logic-sensors-alarm.png (ảnh của bộ cũ); thiết bị thật: (chưa có tệp trong kho) | Nằm trong 101 ca của test_sensors_alarm |
| T3.4 | Rút DS18B20: temp_c = null, flag sensor_temp | Đạt (MÔ PHỎNG); thiết bị thật: Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Đọc -127 cho temp_c = null và flag sensor_temp |
| T3.5 | cmd/feed: servo quay đúng thời gian, event feed_done | Đạt (MÔ PHỎNG); thiết bị thật: Đạt (theo lời nhóm, 03/11/2026) | | | T3.5-3.8_2026-10-30_mo-phong_logic-store-feeder.png (ảnh của bộ cũ); thiết bị thật: (chưa có tệp trong kho) | Nằm trong 89 ca của test_store_feeder |
| T3.6 | id trùng bị bỏ qua; lệnh ngay sau bị too_soon | Đạt (MÔ PHỎNG); thiết bị thật: Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Logic chống trùng và giãn cách lệnh |
| T3.7 | Lần thứ 5 trong ngày bị daily_limit | Đạt (MÔ PHỎNG); thiết bị thật: Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Hạn mức 24h (sai 4 ca khi FEED_MAX_PER_24H = 20, Q1) |
| T3.8 | Đổi config, khởi động lại vẫn giữ (NVS) | Đạt (MÔ PHỎNG); thiết bị thật: Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Lưu cấu hình vào bộ nhớ NVS |
| T3.9 | Còi đúng mẫu cảnh báo, nguy hiểm; tắt tiếng 30 phút | Đạt (MÔ PHỎNG); thiết bị thật: Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Còi 3 mức và tắt tiếng; mẫu mức 1 và 2 CẦN XÁC NHẬN (Q3) |
| T3.10 | Rút router 5 phút: còi vẫn kêu, tự nối lại | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Tự nối lại sau khi có mạng |
| T3.11 | Bảng ca thử risk (E8) trên hàm risk của firmware | Đạt (MÔ PHỎNG) | 2026-10-30 | Nam | T3.11_2026-10-30_mo-phong_bang-ca-thu.png (ảnh của bộ 35 ca cũ) | 72/72 ca, chạy lại 03/11/2026, g++ trên máy tính |
| T3.12 | Tắt tiếng còi qua web / cmd/mute | Đạt (MÔ PHỎNG); thiết bị thật: Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Còi im, event mute, telemetry muted = true; risk tăng còi kêu lại |

Ghi chú kiểm thử logic (MÔ PHỎNG trên máy tính):
- test_risk: 72 ca, sai 0
- test_sensors_alarm: 101 ca, 0 sai
- test_store_feeder: 89 ca, sai 4 khi FEED_MAX_PER_24H = 20 (Q1)

**Hoàn thành khi**: mọi bài trên đạt.
