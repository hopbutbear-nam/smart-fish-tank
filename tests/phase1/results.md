# Giai đoạn 1: Dựng cloud EMQX: kết quả kiểm thử

Cập nhật 03/11/2026 theo kế hoạch bản 1.5. Đổi thành Đạt chỉ khi có bằng chứng; chưa có thì ghi 'Đạt (theo lời nhóm, 03/11/2026)'.

| Mã | Nội dung | Kết quả | Ngày | Người làm | Bằng chứng (tên file trong `evidence/`) | Ghi chú |
| --- | --- | --- | --- | --- | --- | --- |
| T1.1 | Deployment Overview: Running, đúng địa chỉ và cổng | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Xem mục E1 kế hoạch |
| T1.2 | MQTTX webuser mqtts 8883: kết nối, subscribe fishtank/tank01/# | Đạt (qua ESP32; theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Đạt qua ESP32 cổng 8883 |
| T1.3 | webuser subscribe + publish cmd/feed, nhận được tin | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Nhóm xác nhận có ảnh MQTTX |
| T1.4 | Kiểm tra từ chối: publish telemetry và fishtank/other/x bị chặn | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Nhóm xác nhận có ảnh MQTTX |
| T1.5 | MQTTX webuser wss 8084 đường dẫn /mqtt kết nối được | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Nhóm báo đạt wss 8084 |
| T1.6 | Test 4 trên ESP32: WiFi OK, EMQX OK, [OK] mỗi 5 giây | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Nhóm xác nhận có ảnh Serial |
| T1.7 | Retained: nối lại MQTTX nhận ngay tin cuối | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Nhóm xác nhận nhận ngay tin cuối |
| T1.8 | Last Will: rút USB, status thành offline sau 15-30 giây | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | T1.8 bổ sung (quyền tài khoản esp32) bỏ qua có chủ đích 30/10/2026 |
| T1.9 | Ghi số phút kết nối và lưu lượng sau 1 giờ chạy | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Nhóm báo đạt sau 1 giờ chạy |

**Hoàn thành khi**: mọi bài trên đạt.
