# Giai đoạn 2: Kiểm tra và hiệu chuẩn thiết bị: kết quả kiểm thử

Cập nhật 03/11/2026 theo kế hoạch bản 1.5. Đổi thành Đạt chỉ khi có bằng chứng; chưa có thì ghi 'Đạt (theo lời nhóm, 03/11/2026)'.

| Mã | Nội dung | Kết quả | Ngày | Người làm | Bằng chứng (tên file trong `evidence/`) | Ghi chú |
| --- | --- | --- | --- | --- | --- | --- |
| T2.1 | DS18B20: tìm thấy 1 cảm biến, nhiệt độ đổi hợp lý (sai dưới 1 °C so với nhiệt kế) | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Nhiệt độ đổi hợp lý khi nhúng nước ấm; chưa so với nhiệt kế |
| T2.2 | Mực nước: khô, nửa, ngập cho 3 giá trị khác nhau rõ | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | LEVEL_RAW_DRY 0 và LEVEL_RAW_FULL 1560; LEVEL_RAW_SHORT 4090 và LEVEL_SPREAD_MAX 300 tạm |
| T2.3 | Độ đục: điện áp đổi nhất quán, sau chia áp không quá 3,3V | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | TURB_V_CLEAR 3,764 V và TURB_V_DIRTY 2,21 V; sau chia áp an toàn |
| T2.4 | Hiệu chuẩn pH hai điểm, kiểm tra dung dịch thứ ba (sai dưới 0,3) | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | PH_V686 = 2,56 V, PH_V401 = 3,14 V CẦN XÁC NHẬN (chưa commit: 2,66 và 3,24) |
| T2.5 | Servo: tìm SERVO_STOP_US, quay hai chiều | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | SERVO_STOP_US = 1500, SERVO_FEED_US = 1300 (chưa xác nhận đo; chiều quay CẦN XÁC NHẬN) |
| T2.6 | Cơ cấu cho ăn: xác định FEED_MS (5 lần thử chênh dưới 20%) | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | FEED_MS = 600 ms (chưa xác nhận đo) |
| T2.7 | Còi kêu 2 kiểu rõ ràng ở 1300 Hz | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | 1300 Hz (T2.7); duty và chu kỳ theo Q3 (CẦN XÁC NHẬN) |

**Hoàn thành khi**: mọi bài trên đạt.
