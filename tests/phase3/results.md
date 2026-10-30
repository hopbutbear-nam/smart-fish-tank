# Giai đoạn 3: Firmware ESP32: kết quả kiểm thử

Trạng thái ban đầu lấy từ kế hoạch (cập nhật 29/10/2026). Đổi thành **Đạt** chỉ khi có bằng chứng.

| Mã | Nội dung | Kết quả | Ngày | Người làm | Bằng chứng (tên file trong `evidence/`) | Ghi chú |
| --- | --- | --- | --- | --- | --- | --- |
| T3.1 | Biên dịch và nạp, log có phiên bản, WiFi, MQTT, NTP | Chưa | | | | |
| T3.2 | status = online; telemetry mỗi 10 giây, JSON đúng mục E4 | Chưa | | | | |
| T3.3 | Thay đổi từng cảm biến: trường, flags và risk đổi đúng | Chưa | | | | |
| T3.4 | Rút DS18B20: temp_c = null, flag sensor_temp | Chưa | | | | |
| T3.5 | cmd/feed: servo quay đúng thời gian, event feed_done | Chưa | | | | |
| T3.6 | id trùng bị bỏ qua; lệnh ngay sau bị too_soon | Chưa | | | | |
| T3.7 | Lần thứ 5 trong ngày bị daily_limit | Chưa | | | | |
| T3.8 | Đổi config, khởi động lại vẫn giữ (NVS) | Chưa | | | | |
| T3.9 | Còi đúng mẫu cảnh báo, nguy hiểm; tắt tiếng 30 phút | Chưa | | | | |
| T3.10 | Rút router 5 phút: còi vẫn kêu, tự nối lại | Chưa | | | | |
 | T3.11 | Bảng ca thử risk (E8) trên hàm risk của firmware | Đạt (chạy trên máy tính) | 2026-10-30 | Nam | T3.11_2026-10-30_mo-phong_bang-ca-thu.png | 35/35 ca, g++ trên PC; chưa chạy trên ESP32 |

**Hoàn thành khi**: mọi bài trên đạt.
