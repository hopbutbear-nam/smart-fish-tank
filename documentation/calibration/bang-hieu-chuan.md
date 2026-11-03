# Bảng hằng số hiệu chuẩn (đã điền, 03/11/2026)

Firmware phải dùng đúng các hằng số này (đặt trong `firmware/fish_tank/config.h`). Đã đặt `CALIBRATION_DONE = 1` từ ngày 01/11/2026 (commit `e7f56f3`).

| Hằng số | Ý nghĩa | Giá trị đo | Nguồn | Ngày đo | Người đo |
| --- | --- | --- | --- | --- | --- |
| V686 | Điện áp module pH trong dung dịch 6,86 (V) | 2,56 V (chưa commit: 2,66 V) CẦN XÁC NHẬN | config.h | | |
| V401 | Điện áp module pH trong dung dịch 4,01 (V) | 3,14 V (chưa commit: 3,24 V) CẦN XÁC NHẬN | config.h | | |
| TURB_V_CLEAR | Điện áp module độ đục trong nước sạch (V) | 3,764 V | config.h | | |
| TURB_V_DIRTY | Điện áp trong nước rất đục (V) | 2,21 V | config.h | | |
| LEVEL_DRY | Giá trị thô khi khô (LEVEL_RAW_DRY) | 0 | config.h | | |
| LEVEL_FULL | Giá trị thô khi ngập tới mức đầy (LEVEL_RAW_FULL) | 1560 | config.h | | |
| LEVEL_RAW_SHORT | Ngưỡng phát hiện chập cảm biến mực nước | 4090 (giá trị tạm) | config.h | | |
| LEVEL_SPREAD_MAX | Độ lệch mẫu tối đa cho phép | 300 (giá trị tạm) | config.h | | |
| SERVO_STOP_US | Xung làm servo đứng yên (µs) | 1500 µs (chưa xác nhận đo) | config.h | | |
| SERVO_FEED_US | Xung làm servo quay cho ăn (µs) | 1300 µs (chưa xác nhận đo) | config.h | | |
| FEED_MS | Thời gian servo quay cho một phần ăn (ms) | 600 ms | config.h | | |
| Chiều quay đúng | 1300 hay 1700 đưa thức ăn ra | CẦN XÁC NHẬN | config.h | | |
| BUZZ_FREQ_HZ | Tần số còi phát ra âm thanh to nhất | 1300 Hz | config.h | | |
| BUZZ_DUTY_QUIET_PCT | Độ rộng xung duty cycle mức 1 và 2 | 50% (kế hoạch: 15%) CẦN XÁC NHẬN | config.h | | |
| BUZZ_DUTY_LOUD_PCT | Độ rộng xung duty cycle mức 3 | 50% | config.h | | |

Công thức pH: `pH = 6.86 + (V - V686) * (6.86 - 4.01) / (V686 - V401)`

Phiên bản gói esp32 đã dùng: [nhóm điền]
