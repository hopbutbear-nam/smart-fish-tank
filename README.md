# smart-fish-tank: Hệ thống quản lý bể cá thông minh (IoT)

> **Cập nhật:** 31/10/2026 · Theo kế hoạch dự án phiên bản 1.3  
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
| Loài cá mục tiêu | Cá guppy; bể mini để bàn khoảng 4–6 lít |
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

Trạng thái dưới đây phản ánh kế hoạch phiên bản 1.3, cập nhật ngày 31/10/2026. Một giai đoạn chỉ được xem là hoàn thành khi các bài kiểm thử bắt buộc của giai đoạn đó có kết quả và bằng chứng. **Code đã viết hoặc chạy mô phỏng không đồng nghĩa hệ thống đã chạy đạt trên phần cứng thật.**

| GĐ | Tên | Trạng thái đến 31/10/2026 |
| --- | --- | --- |
| 0 | Chuẩn bị môi trường | **Cơ bản hoàn tất.** Đã cài công cụ/thư viện, nạp chương trình thử, tạo kho và mời thành viên. Nhóm đã báo xử lý sự cố bí mật và đổi mật khẩu; vẫn cần chạy lại rà soát kho (T0.4). |
| 1 | Dựng cloud EMQX | **Gần hoàn tất.** Đã tạo deployment/tài khoản, cấu hình quyền, thử MQTT/TLS và tải CA. Còn T1.9; bài kiểm tra bổ sung T1.8 về quyền tài khoản `esp32` đã được nhóm chủ động bỏ qua nên chưa có bằng chứng. |
| 2 | Kiểm tra và hiệu chuẩn thiết bị | **Đang làm, bị giới hạn bởi phần cứng đang cho mượn.** DS18B20 đã đọc được; servo đã quay; còi đã kêu. Còn kiểm tra mực nước, pH, độ đục, cơ cấu cho ăn, so nhiệt kế và hoàn thiện bảng hiệu chuẩn. |
| 3 | Firmware ESP32 | **Đã viết code; kiểm thử logic MÔ PHỎNG đạt.** Risk: 35/35 ca; logic cảm biến/còi: 83 ca, 0 lỗi; logic cho ăn/lưu cấu hình: 76 ca, 0 lỗi. Chưa có bằng chứng biên dịch, nạp và chạy trên ESP32 thật; hằng số hiệu chuẩn còn tạm. |
| 4 | Web frontend | **Đã viết code; đã thử với ESP32 giả (MÔ PHỎNG).** Có ảnh cho một phần bài thử, nhưng chưa đủ bằng chứng để kết luận toàn bộ T4.1–T4.9 đạt. Chưa thử với ESP32 thật và GitHub Pages chưa bật. |
| 5 | Ghép nối, thử nghiệm toàn hệ thống | **Chưa làm.** Cần thực hiện các kịch bản T5.1–T5.10, gồm cảm biến, cho ăn, cảnh báo, mất mạng, chạy liên tục và truy cập từ mạng di động. |
| 6 | Đưa web lên mạng, bảo mật | **Chưa làm.** Cần bật GitHub Pages, chuyển firmware sang xác minh TLS bằng CA, kiểm tra ACL, rà soát bí mật và hoàn tất các bài T6.1–T6.5. |
| 7 | Tính năng mở rộng | **Tùy chọn.** Thứ tự ưu tiên đề xuất: Telegram, vỏ hộp/chống nước, lưu lịch sử dài hạn, OTA. |
| 8 | Hoàn thiện và nghiệm thu | **Chưa làm.** Mục tiêu là chạy trên bể thật 3–7 ngày, hoàn thiện tài liệu, báo cáo và demo. |

### Việc cần ưu tiên tiếp theo

1. Nhận lại ESP32 và các linh kiện; hoàn thành Giai đoạn 2, đặc biệt hiệu chuẩn pH, độ đục, mực nước, servo và cơ cấu cho ăn. Điền các số đo vào `documentation/calibration/bang-hieu-chuan.md`.
2. Biên dịch/nạp firmware trên ESP32 thật; kiểm tra Wi-Fi, NTP, MQTT, cảm biến, còi và servo. Thay các hằng số tạm trong `firmware/fish_tank/config.h` bằng số liệu hiệu chuẩn đã đo.
3. Cập nhật kết quả và bằng chứng của các bài đã chạy vào `tests/phase3/results.md` và `tests/phase4/results.md`; lưu ảnh/log đúng thư mục `evidence/`. Ghi rõ bài nào chỉ là MÔ PHỎNG.
4. Thực hiện đủ các bài kiểm thử tích hợp T5.1–T5.10 trước khi coi hệ thống hoạt động hoàn chỉnh.
5. Thực hiện Giai đoạn 6: bật GitHub Pages, thay chế độ TLS tạm bằng xác minh CA trong firmware, rà soát lại ACL và quét toàn bộ kho để chắc chắn không còn bí mật.

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
├── documentation/
│   ├── IoT-fish-tank-plan.docx
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

## Bảo mật và giới hạn cần biết

- Không commit `secrets.h`, mật khẩu Wi-Fi/MQTT, token Telegram hoặc API key. `secrets.example.h` chỉ chứa giá trị giả để minh họa cấu trúc.
- Kế hoạch ghi nhận đã từng có thông tin đăng nhập thật trong `firmware/fish_tank/secrets.example.h`; nhóm báo đã sửa tệp và đổi mật khẩu. Vì kho công khai có lịch sử commit, vẫn phải kiểm tra lại cả nội dung hiện tại và lịch sử kho; nếu phát hiện bí mật từng bị đẩy lên, tiếp tục thay/thu hồi bí mật tương ứng.
- Firmware ở trạng thái Giai đoạn 3 đang dùng cấu hình TLS tạm `setInsecure()`. Giai đoạn 6 phải thay bằng xác minh chứng chỉ CA (`setCACert`) và đồng bộ NTP trước khi kết nối.
- Web là frontend tĩnh, **không có hệ thống xác thực người dùng thực sự**. Người biết thông tin đăng nhập MQTT `webuser` có thể xem dữ liệu và gửi lệnh điều khiển. Muốn có tài khoản riêng và phân quyền nhiều người dùng cần bổ sung backend.
- Giữ Spend Limit EMQX bằng `0` theo kế hoạch; không tạo API key nếu dự án không cần.

## Tài liệu và liên kết

- **Kế hoạch dự án (nguồn thông số chuẩn):** [`documentation/IoT-fish-tank-plan.docx`](documentation/IoT-fish-tank-plan.docx)
- **Phân vai nhóm:** [`documentation/team/phan-vai.md`](documentation/team/phan-vai.md)
- **Firmware:** [`firmware/README.md`](firmware/README.md)
- **Kho mã nguồn:** [GitHub – smart-fish-tank](https://github.com/hopbutbear-nam/smart-fish-tank)
- **Clone kho:** `git clone https://github.com/hopbutbear-nam/smart-fish-tank.git`
- **GitHub Pages (đường dẫn dự kiến, chưa bật):** <https://hopbutbear-nam.github.io/smart-fish-tank/>

## Quy tắc lưu tệp và commit

Mọi code, tài liệu, sơ đồ, ảnh kiểm thử và báo cáo phải được lưu vào đúng thư mục trong kho; không để tệp chỉ nằm trong cuộc trò chuyện. Sau mỗi bài kiểm thử, cập nhật `tests/phaseN/results.md` cùng ảnh/log trong `tests/phaseN/evidence/` và che mọi thông tin nhạy cảm trong bằng chứng.

Trước khi commit, chạy `git pull`, kiểm tra `git status` và chắc chắn không có bí mật nào được thêm vào. Không dùng `git push --force` hoặc xóa lịch sử nếu chưa được cả nhóm đồng ý.

Mẫu commit: `<phần>: <việc đã làm>`, trong đó `<phần>` là `firmware`, `web`, `tests`, `docs`, `tools` hoặc `extensions`.

Ví dụ:

```bash
git pull
git status
git add .
git commit -m "firmware: cập nhật logic cảnh báo"
git push
```
