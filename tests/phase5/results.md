# Giai đoạn 5: Ghép nối, thử nghiệm toàn hệ thống: kết quả kiểm thử

Cập nhật 03/11/2026 theo kế hoạch bản 1.5. Đổi thành Đạt chỉ khi có bằng chứng; chưa có thì ghi 'Đạt (theo lời nhóm, 03/11/2026)'.

| Mã | Nội dung | Kết quả | Ngày | Người làm | Bằng chứng (tên file trong `evidence/`) | Ghi chú |
| --- | --- | --- | --- | --- | --- | --- |
| T5.1 | Bật nguồn ESP32: web hiện số trong khoảng 10 giây | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Lên WiFi, web nhận telemetry sau bật nguồn |
| T5.2 | Nhúng cảm biến nhiệt vào nước ấm: web đổi, cảnh báo nếu vượt ngưỡng | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Nhiệt độ tăng, cờ và màu cảnh báo đổi |
| T5.3 | Bấm Cho ăn trên web: servo quay, web nhận xác nhận | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Servo quay 600 ms, event feed_done |
| T5.4 | Nhỏ dung dịch pH khác: còi và web cùng cảnh báo | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Còi kêu và web hiện cảnh báo pH |
| T5.5 | Rút router, cắm lại: còi vẫn kêu cục bộ, tự nối lại | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Còi kêu cục bộ không phụ thuộc mạng, tự nối lại |
| T5.6 | Tắt nguồn ESP32: web báo offline nhờ Last Will | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Last Will hoạt động khi tắt nguồn |
| T5.7 | Chạy liên tục 24-48 giờ: không tự khởi động lại, không treo | Chưa | | | (chưa có tệp trong kho) | Chờ nhóm điền số giờ chạy thực tế (theo A10) |
| T5.8 | Thử từ nơi khác bằng 4G: xem và điều khiển được | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Thử nghiệm qua mạng di động |
| T5.9 | webuser publish vào telemetry: bị chặn | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Whitelist ACL của EMQX chặn quyền publish |
| T5.10 | Mất điện rồi có điện lại: ESP32 tự lên, giữ ngưỡng và bộ đếm cho ăn | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Lưu trữ NVS bảo toàn cấu hình |

**Hoàn thành khi**: mọi bài trên đạt.
