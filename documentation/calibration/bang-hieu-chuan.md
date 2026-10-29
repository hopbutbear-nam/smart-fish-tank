# Bảng hằng số hiệu chuẩn (điền sau Giai đoạn 2)

Firmware phải dùng đúng các hằng số này (đặt trong `firmware/fish_tank/config.h`).

| Hằng số | Ý nghĩa | Giá trị đo | Ngày đo | Người đo |
| --- | --- | --- | --- | --- |
| V686 | Điện áp module pH trong dung dịch 6,86 (V) | | | |
| V401 | Điện áp module pH trong dung dịch 4,01 (V) | | | |
| TURB_V_CLEAR | Điện áp module độ đục trong nước sạch (V) | | | |
| TURB_V_DIRTY | Điện áp trong nước rất đục (V) | | | |
| LEVEL_DRY | Giá trị thô khi khô | | | |
| LEVEL_FULL | Giá trị thô khi ngập tới mức đầy | | | |
| SERVO_STOP_US | Xung làm servo đứng yên (µs) | | | |
| FEED_MS | Thời gian servo quay cho một phần ăn (ms) | | | |
| Chiều quay đúng | a (1300) hay d (1700) đưa thức ăn ra | | | |

Công thức pH: `pH = 6.86 + (V - V686) * (6.86 - 4.01) / (V686 - V401)`

Phiên bản gói esp32 đã dùng: ...
