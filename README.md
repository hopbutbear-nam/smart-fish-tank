# smart-fish-tank: Hệ thống quản lý bể cá thông minh (IoT)

> **Cập nhật:** 03/11/2026 · Theo kế hoạch dự án phiên bản 1.5  
> **Kho mã nguồn:** [hopbutbear-nam/smart-fish-tank](https://github.com/hopbutbear-nam/smart-fish-tank)

Hệ thống sử dụng ESP32 để theo dõi nhiệt độ, pH, độ đục và mực nước của bể cá; cảnh báo tại chỗ bằng còi; điều khiển cơ cấu cho ăn bằng servo; đồng thời gửi dữ liệu qua Wi-Fi lên EMQX Cloud để người nuôi theo dõi và gửi lệnh từ xa qua web.

Kho GitHub này là **công khai**. Tuyệt đối không đưa mật khẩu Wi-Fi, mật khẩu MQTT, token Telegram, API key hoặc bất kỳ bí mật nào vào kho.

## Tổng quan dự án

| Hạng mục | Thông tin |
| --- | --- |
| Nền tảng | ESP32 + EMQX Cloud Serverless (MQTT) + web tĩnh (HTML/CSS/JavaScript) |
| Kiến trúc | ESP32 ↔ EMQX Cloud ↔ web frontend; **không có backend riêng** |
| Giao tiếp | MQTT qua TLS; trình duyệt kết nối bằng WebSocket bảo mật (WSS) |
| Cảm biến | DS18B20, cảm biến pH, cảm biến độ đục, cảm biến mực nước |
| Cơ cấu chấp hành | Servo SG90 loại xoay liên tục 360° và còi thụ động |
| Loài cá mục tiêu | Cá guppy; bể mini để bàn khoảng 4 đến 6 l |
| Quy mô nhóm | 3 thành viên; phân công cơ khí/kiểm tra linh kiện, firmware/web, tích hợp/kiểm thử |
| Ngân sách đã ghi nhận | Khoảng 1.151.000 VNĐ |
| Hạn nộp theo kế hoạch | 03/11/2026 |

### Chức năng chính

- Đọc và hiển thị nhiệt độ, pH, độ đục và mực nước.
- Đánh giá rủi ro theo nhiều chỉ số, hiển thị trạng thái và cảnh báo bằng còi tại bể.
- Cho ăn bằng servo theo lệnh từ web hoặc lịch đã cấu hình; firmware áp dụng giới hạn chống cho ăn quá thường xuyên.
- Gửi telemetry và sự kiện qua MQTT; web nhận trạng thái online/offline, hiển thị dữ liệu và cho phép cập nhật cấu hình.
- Giao diện web có biểu đồ 24 giờ, chế độ tối và hỗ trợ PWA.

Ngoài phạm vi bắt buộc: Telegram, lưu lịch sử dài hạn, OTA và các tính năng cần hệ thống tài khoản/backend riêng. Các mục này chỉ triển khai nếu còn thời gian.

## Kiến trúc và tài liệu kỹ thuật

Luồng dữ liệu tổng quát:

```text
Chiều dữ liệu: Cảm biến → ESP32 → EMQX Cloud → Web frontend
Chiều điều khiển: Web frontend → EMQX Cloud → ESP32 → Servo
                                                      └→ Còi cảnh báo
```

- **Firmware:** Arduino/C++ trong `firmware/fish_tank/`.
- **Broker:** EMQX Cloud Serverless. ESP32/MQTTX sử dụng MQTT TLS; web sử dụng WSS.
- **Web:** trang tĩnh trong `docs/`, dự kiến xuất bản bằng GitHub Pages.
- **ESP32 giả:** `tools/fake-esp32/fake_esp32.py` dùng để thử luồng MQTT/web khi chưa có phần cứng. Kết quả từ công cụ này phải ghi rõ là **MÔ PHỎNG**, không thay cho kiểm thử trên ESP32 thật.

### Nguồn thông số chuẩn

`documentation/IoT-fish-tank-plan.docx` là tài liệu kế hoạch chi tiết. **Mục E – Thông số cố định là nguồn sự thật duy nhất** cho các thông tin sau:

- Host, cổng, tài khoản và chứng chỉ EMQX.
- Topic MQTT, quyền publish/subscribe (ACL) và định dạng JSON.
- Sơ đồ chân GPIO, cách cấp nguồn và cầu chia áp.
- Ngưỡng cảnh báo, luật tính `risk`, cấu hình và giới hạn cho ăn.

Không tự ý đổi các thông số này riêng trong README, firmware hoặc web. Khi cần thay đổi, cập nhật mục E trong kế hoạch trước rồi đồng bộ code và tài liệu liên quan.

Một số lưu ý phần cứng quan trọng:

- ESP32 dùng mức logic 3,3 V; không nối trực tiếp ngõ ra analog 5 V vào GPIO. Module pH và độ đục phải qua cầu chia áp theo sơ đồ mục E5.
- Servo cần nguồn 5 V phù hợp; không cấp nguồn servo từ chân 3,3 V. Phải nối chung GND giữa ESP32, servo và các module.
- Servo SG90 đã mua là loại **360° xoay liên tục**, điều khiển bằng thời gian quay; không điều khiển đặt góc như servo 180°.
- Còi là loại thụ động; tần số đã chọn trong kế hoạch là 1300 Hz.
- Cảm biến mực nước chỉ được cấp nguồn trong thời gian ngắn lúc đo để hạn chế ăn mòn điện cực.

## Tiến độ dự án

Trạng thái dưới đây phản ánh kế hoạch phiên bản 1.5, cập nhật ngày 03/11/2026. Một giai đoạn chỉ được xem là hoàn thành khi các bài kiểm thử bắt buộc của giai đoạn đó có kết quả và bằng chứng. **Code đã viết hoặc chạy mô phỏng không đồng nghĩa hệ thống đã chạy đạt trên phần cứng thật.** Mục ghi 'theo lời nhóm' chưa có bằng chứng trong tests/.

| GĐ | Tên | Trạng thái đến 03/11/2026 |
| --- | --- | --- |
| 0 | Chuẩn bị môi trường | **Hoàn tất (theo lời nhóm, 03/11/2026).** T0.1 đến T0.4 đạt; đã cài công cụ/thư viện, nạp chương trình thử, tạo kho và phân vai. |
| 1 | Dựng cloud EMQX | **Hoàn tất (theo lời nhóm, 03/11/2026).** T1.1 đến T1.9 đạt; đã tạo deployment/tài khoản, cấu hình whitelist, thử MQTT/TLS và tải CA. T1.8 bổ sung đã bỏ qua có chủ đích. |
| 2 | Kiểm tra và hiệu chuẩn thiết bị | **Hiệu chuẩn xong: CALIBRATION_DONE = 1 (01/11/2026), số đo ở documentation/calibration/bang-hieu-chuan.md.** Bộ kit đã nối hoàn chỉnh và kiểm thử đạt (theo lời nhóm, 03/11/2026). |
| 3 | Firmware ESP32 | **Logic MÔ PHỎNG: risk 72/72, cảm biến và còi 101 ca 0 lỗi, cho ăn và lưu cấu hình 89 ca (sai 4 khi FEED_MAX_PER_24H = 20, xem Q1); chạy thật đạt (theo lời nhóm, 03/11/2026).** Bộ ca thử chạy lại 03/11/2026; ảnh trong tests/phase3/evidence/ là của bộ cũ 35, 83, 76 ca. |
| 4 | Web frontend | **Code đã viết; mô phỏng ESP32 giả đạt; chạy thật đạt (theo lời nhóm, 03/11/2026); GitHub Pages chưa bật.** Đã thử nghiệm các chức năng hiển thị, điều khiển cho ăn và cảnh báo. |
| 5 | Ghép nối, thử nghiệm toàn hệ thống | **Đạt (theo lời nhóm, 03/11/2026); riêng T5.7 chạy liên tục nhóm điền số giờ.** Đã thử nghiệm cảm biến, cho ăn, còi, mất mạng và kết nối 4G. |
| 6 | Đưa web lên mạng, bảo mật | **Một phần: đã đổi mật khẩu, whitelist bật; CA và GitHub Pages chưa làm (hạn chế đã biết).** secrets.example.h đã giá trị giả (03/11/2026); rà soát bí mật đạt theo lời nhóm. |
| 7 | Tính năng mở rộng | **Chưa thực hiện (tính năng mở rộng).** Đề xuất: Telegram, vỏ hộp/chống nước, lưu lịch sử dài hạn, OTA. |
| 8 | Hoàn thiện và nghiệm thu | **Đang hoàn thiện: bảng nghiệm thu đã ánh xạ; chờ số liệu chạy thật (A10) và link báo cáo/slide (A11).** Nghiệm thu đạt 5/9 tiêu chí. |

### Việc còn lại trước khi nộp

1. Chốt các câu hỏi kỹ thuật còn mở (Q1: hạn mức cho ăn 4 hay 20; Q2: pH 2,56/3,14 hay 2,66/3,24; Q3: duty/chu kỳ còi mức 1 và 2; Q4: chuyển TLS CA trong firmware).
2. Bài chỉ có ghi nhận "theo lời nhóm": nhóm bổ sung ảnh chụp màn hình và log Serial vào `tests/phaseN/evidence/`.
3. Bổ sung đường dẫn báo cáo, slide thuyết trình và video demo hệ thống (mục A11).
4. Bật GitHub Pages cho thư mục `docs/` và chuyển firmware sang dùng `setCACert` nếu hoàn thiện bảo mật Giai đoạn 6.

## Cấu trúc kho

```text
smart-fish-tank/
├── .gitignore
├── README.md
├── firmware/
│   ├── README.md
│   ├── certs/
│   │   └── emqx-ca.pem
│   ├── tests/
│   │   ├── reset_feed_history/
│   │   ├── test1_blink/
│   │   ├── test4_emqx/
│   │   ├── test_ds18b20/
│   │   ├── test_level/
│   │   ├── test_analog/
│   │   ├── test_servo/
│   │   └── test_buzzer/
│   └── fish_tank/
│       ├── fish_tank.ino
│       ├── config.h
│       ├── secrets.example.h
│       ├── sensors.h / sensors.cpp
│       ├── risk.h / risk.cpp
│       ├── alarm.h / alarm.cpp
│       ├── feeder.h / feeder.cpp
│       ├── net.h / net.cpp
│       └── store.h / store.cpp
├── docs/                         # Web frontend và GitHub Pages
│   ├── index.html
│   ├── style.css
│   ├── app.js
│   ├── manifest.json
│   ├── sw.js
│   └── icons/
│       └── icon.svg
├── documentation/
│   ├── IoT-fish-tank-plan.docx
│   ├── fix-plan.md
│   ├── fixing.docx
│   ├── team/                     # phan-vai.md
│   ├── diagrams/
│   ├── calibration/              # bang-hieu-chuan.md
│   ├── hardware/
│   ├── guides/
│   └── report/
├── tests/
│   ├── phase0/ ... phase6/       # results.md và evidence/
│   ├── phase7/                   # README.md và evidence/
│   └── phase8/                   # nghiệm thu và nhật ký chạy thật
│       ├── nghiem-thu.md
│       └── nhat-ky-chay-that.md
├── tools/
│   ├── mqtt-samples/
│   ├── fake-esp32/               # fake_esp32.py
│   ├── test-risk/                # test_risk.cpp
│   ├── test-sensors-alarm/       # test_sensors_alarm.cpp
│   └── test-store-feeder/        # test_store_feeder.cpp
└── extensions/
    ├── telegram/
    ├── enclosure/
    ├── history/
    └── ota/
```

Các thư mục `evidence/` có thể chứa `.gitkeep` để Git giữ thư mục trống. Không đưa `secrets.h` thật lên GitHub; tệp này chỉ tồn tại trên máy của nhóm và phải được `.gitignore` loại trừ.

## Môi trường phát triển và kiểm thử

- **Firmware:** Arduino IDE 2.x; board `ESP32 Dev Module`; Serial Monitor 115200 baud.
- **Thư viện Arduino:** OneWire, DallasTemperature, PubSubClient, ArduinoJson 7.x và ESP32Servo.
- **Thử MQTT:** MQTTX; thông tin kết nối và topic xem tại mục E của kế hoạch.
- **ESP32 giả:** Python 3.8 trở lên và `paho-mqtt`; mã nguồn tại `tools/fake-esp32/`.
- **Kiểm thử logic trên máy tính:** các chương trình trong `tools/test-risk/`, `tools/test-sensors-alarm/` và `tools/test-store-feeder/`.
  - `tools/test-risk/test_risk.cpp`:
    `g++ -std=c++11 -Wall -I firmware/fish_tank tools/test-risk/test_risk.cpp firmware/fish_tank/risk.cpp -o test_risk`
    Kết quả mong đợi: Tổng: 72 ca, sai 0.
  - `tools/test-sensors-alarm/test_sensors_alarm.cpp`:
    `g++ -std=c++11 -Wall -I firmware/fish_tank tools/test-sensors-alarm/test_sensors_alarm.cpp firmware/fish_tank/sensors.cpp firmware/fish_tank/alarm.cpp firmware/fish_tank/risk.cpp -o tools/test-sensors-alarm/test_sensors_alarm.exe`
    Kết quả mong đợi: 101 ca, sai 0.
  - `tools/test-store-feeder/test_store_feeder.cpp`:
    `g++ -std=c++11 -Wall -I firmware/fish_tank tools/test-store-feeder/test_store_feeder.cpp firmware/fish_tank/store.cpp firmware/fish_tank/feeder.cpp firmware/fish_tank/risk.cpp -o tools/test-store-feeder/test_store_feeder.exe`
    Kết quả mong đợi: 89 ca, sai 0 (sai 4 ca khi FEED_MAX_PER_24H = 20 theo Q1).

## Bảo mật và giới hạn cần biết

- Không commit `secrets.h`, mật khẩu Wi-Fi/MQTT, token Telegram hoặc API key. `secrets.example.h` chỉ chứa giá trị giả để minh họa cấu trúc.
- `secrets.example.h` hiện chỉ chứa giá trị giả (đã kiểm 03/11/2026). Lịch sử git còn bản cũ; nhóm báo đã đổi mật khẩu WiFi và tài khoản esp32; rà soát lại: Đạt (theo lời nhóm, 03/11/2026).
- Firmware ở trạng thái Giai đoạn 3 đang dùng cấu hình TLS tạm `setInsecure()`; chưa chuyển sang `setCACert` tại 03/11/2026 (theo Q4).
- Web là frontend tĩnh, **không có hệ thống xác thực người dùng thực sự**. Người biết thông tin đăng nhập MQTT `webuser` có thể xem dữ liệu và gửi lệnh điều khiển. Muốn có tài khoản riêng và phân quyền nhiều người dùng cần bổ sung backend.
- Giữ Spend Limit EMQX bằng `0` theo kế hoạch; không tạo API key nếu dự án không cần.

## Tài liệu và liên kết

- **Kế hoạch dự án (nguồn thông số chuẩn bản 1.5):** [`documentation/IoT-fish-tank-plan.docx`](documentation/IoT-fish-tank-plan.docx)
- **Kế hoạch cập nhật tài liệu bản 1.5:** [`documentation/fixing.docx`](documentation/fixing.docx)
- **Lịch sử sửa bản 1.4:** [`documentation/fix-plan.md`](documentation/fix-plan.md)
- **Báo cáo, slide và video demo:** (chưa có trong kho, nhóm bổ sung đường dẫn)
- **Phân vai nhóm:** [`documentation/team/phan-vai.md`](documentation/team/phan-vai.md)
- **Firmware:** [`firmware/README.md`](firmware/README.md)
- **Kho mã nguồn:** [GitHub – smart-fish-tank](https://github.com/hopbutbear-nam/smart-fish-tank)
- **Clone kho:** `git clone https://github.com/hopbutbear-nam/smart-fish-tank.git`
- **GitHub Pages (đường dẫn dự kiến, chưa bật):** <https://hopbutbear-nam.github.io/smart-fish-tank/>

## Quy tắc lưu tệp và commit

Mọi code, tài liệu, sơ đồ, ảnh kiểm thử và báo cáo phải được lưu vào đúng thư mục trong kho; không để tệp chỉ nằm trong cuộc trò chuyện. Sau mỗi bài kiểm thử, cập nhật `tests/phaseN/results.md` cùng ảnh/log trong `tests/phaseN/evidence/` và che mọi thông tin nhạy cảm trong bằng chứng.

Khi git status liệt kê nhiều tệp chỉ khác kết thúc dòng, không dùng git add .; thêm từng tệp cần commit.

Trước khi commit, chạy `git pull`, kiểm tra `git status` và chắc chắn không có bí mật nào được thêm vào. Không dùng `git push --force` hoặc xóa lịch sử nếu chưa được cả nhóm đồng ý.

Mẫu commit: `<phần>: <việc đã làm>`, trong đó `<phần>` là `firmware`, `web`, `tests`, `docs`, `tools` hoặc `extensions`.

Ví dụ:

```bash
git pull
git status
git add documentation/IoT-fish-tank-plan.docx
git commit -m "firmware: cập nhật logic cảnh báo"
git push
```
