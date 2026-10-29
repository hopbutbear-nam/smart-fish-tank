# Giai đoạn 1: Dựng cloud EMQX: kết quả kiểm thử

Trạng thái ban đầu lấy từ kế hoạch (cập nhật 29/10/2026). Đổi thành **Đạt** chỉ khi có bằng chứng.

| Mã | Nội dung | Kết quả | Ngày | Người làm | Bằng chứng (tên file trong `evidence/`) | Ghi chú |
| --- | --- | --- | --- | --- | --- | --- |
| T1.1 | Deployment Overview: Running, đúng địa chỉ và cổng | Chưa ghi | | | | |
| T1.2 | MQTTX webuser mqtts 8883: kết nối, subscribe fishtank/tank01/# | Đạt (qua ESP32) | | | | |
| T1.3 | webuser subscribe + publish cmd/feed, nhận được tin | Đạt (có ảnh MQTTX) | | | | |
| T1.4 | Kiểm tra từ chối: publish telemetry và fishtank/other/x bị chặn | Đạt (có ảnh MQTTX) | | | | |
| T1.5 | MQTTX webuser wss 8084 đường dẫn /mqtt kết nối được | Theo lời nhóm, chưa có ảnh | | | | |
| T1.6 | Test 4 trên ESP32: WiFi OK, EMQX OK, [OK] mỗi 5 giây | Đạt (có ảnh Serial) | | | | |
| T1.7 | Retained: nối lại MQTTX nhận ngay tin cuối | Đạt (có ảnh) | | | | |
| T1.8 | Last Will: rút USB, status thành offline sau 15-30 giây | Theo lời nhóm | | | | |
| T1.9 | Ghi số phút kết nối và lưu lượng sau 1 giờ chạy | Chưa | | | | |

**Hoàn thành khi**: mọi bài trên đạt.
