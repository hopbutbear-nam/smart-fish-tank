# PHÂN CÔNG NHIỆM VỤ NHÓM

**Dự án:** Hệ thống quản lý bể cá thông minh (IoT-fish-tank)  
**Kho mã nguồn:** <https://github.com/hopbutbear-nam/smart-fish-tank>  
**Ngày cập nhật:** 31/10/2026  
**Hạn nộp dự án:** 03/11/2026

> Tài liệu này được lập theo kế hoạch `IoT-fish-tank-plan` phiên bản 1.3. Nhóm có 3 thành viên. Kế hoạch hiện chưa cung cấp họ tên cụ thể, vì vậy các vị trí tên bên dưới được để trống để nhóm tự điền.

## 1. Danh sách thành viên

| Thành viên | Họ và tên | Vai trò chính |
|---|---|---|
| Thành viên 1 | `[Điền họ tên]` | Cơ khí, lắp ráp và kiểm tra kết nối từng linh kiện |
| Thành viên 2 | `[Điền họ tên]` | Phát triển firmware ESP32 và web frontend |
| Thành viên 3 | `[Điền họ tên]` | Tích hợp hệ thống và kiểm thử |

**Lưu ý về cách phân công:** Mục B1 của kế hoạch có cách mô tả vai trò ban đầu là “firmware và web, cloud và kiểm thử, cơ khí”. Phần H2 ghi nhận câu trả lời cập nhật của nhóm: một thành viên phụ trách cơ khí và kiểm tra kết nối linh kiện, một thành viên phụ trách firmware và web, thành viên còn lại phụ trách tích hợp và kiểm thử. Bảng phân công trong tài liệu này sử dụng cách xác nhận mới hơn tại mục H2. Nếu nhóm muốn có một đầu mối phụ trách riêng EMQX Cloud, cần thống nhất và bổ sung người phụ trách.

## 2. Nhiệm vụ theo từng thành viên

### Thành viên 1 – Cơ khí, lắp ráp và kiểm tra linh kiện

**Mục tiêu:** Hoàn thiện phần cứng, đấu nối đúng sơ đồ và cung cấp số liệu thực tế để hiệu chuẩn hệ thống.

**Công việc chính:**

- Kiểm tra bộ kit ESP32, đế mở rộng, cảm biến, servo, còi, dây nối và các đầu nối trước khi lắp.
- Lắp ráp phần cứng theo sơ đồ chân tại mục E5 của kế hoạch; phối hợp kiểm tra từng đường dây trước khi cấp nguồn.
- Kiểm tra cảm biến và cơ cấu chấp hành theo các bài thử Giai đoạn 2:
  - T2.1: DS18B20 đo nhiệt độ.
  - T2.2: Cảm biến mực nước.
  - T2.3: Module đo độ đục.
  - T2.4: Đầu dò pH và hiệu chuẩn hai điểm.
  - T2.5: Servo SG90 loại xoay liên tục 360°.
  - T2.6: Cơ cấu cho ăn và lượng thức ăn mỗi phần.
  - T2.7: Còi cảnh báo ở tần số 1300 Hz.
- Ghi lại các giá trị đo và thông số hiệu chuẩn, gồm V686, V401, TURB_V_CLEAR, TURB_V_DIRTY, LEVEL_DRY, LEVEL_FULL, SERVO_STOP_US, FEED_MS và chiều quay servo.
- Cập nhật bảng hiệu chuẩn tại `documentation/calibration/bang-hieu-chuan.md` sau khi đo được số liệu thực tế.
- Phối hợp chuẩn bị hộp bảo vệ, bố trí dây và cố định đầu dò để hạn chế nước bắn vào mạch.
- Chụp ảnh quá trình lắp ráp và kết quả kiểm tra; lưu bằng chứng vào thư mục `tests/phase2/evidence/` hoặc thư mục giai đoạn tương ứng.

**Sản phẩm bàn giao:**

- Bộ phần cứng được lắp và đấu nối theo sơ đồ.
- Kết quả kiểm thử T2.1–T2.7 và ảnh/log bằng chứng.
- Bảng hiệu chuẩn có số liệu thực tế.
- Sơ đồ đấu dây, ảnh bộ kit và ghi chú lắp ráp lưu trong `documentation/diagrams/` và `documentation/hardware/`.

**Lưu ý an toàn:**

- Không đưa tín hiệu 5V trực tiếp vào GPIO ESP32; ngõ ra analog 5V của module pH và độ đục phải qua cầu chia áp theo mục E5.
- Dùng chung GND giữa ESP32, servo và các module; không cấp nguồn servo từ chân 3,3V.
- Cảm biến mực nước chỉ được cấp nguồn trong khoảng thời gian đo theo thiết kế.
- Giữ đầu dò pH luôn ẩm; tránh để đầu dò khô.

### Thành viên 2 – Firmware ESP32 và web frontend

**Mục tiêu:** Hoàn thiện phần mềm trên ESP32 và giao diện web, bảo đảm hai phía sử dụng đúng thông số giao tiếp đã chốt.

**Firmware:**

- Phát triển và duy trì chương trình trong `firmware/fish_tank/`.
- Phụ trách các thành phần theo cấu trúc dự án: `fish_tank.ino`, `config.h`, `sensors.h/.cpp`, `risk.h/.cpp`, `alarm.h/.cpp`, `feeder.h/.cpp`, `net.h/.cpp` và `store.h/.cpp`.
- Dùng đúng broker, topic MQTT, định dạng JSON, GPIO và ngưỡng mặc định tại mục E của kế hoạch; không tự ý đổi các thông số này.
- Hoàn thiện đọc cảm biến, tính risk và flags, cảnh báo cục bộ, cho ăn bằng servo, lưu cấu hình NVS, đồng bộ NTP và kết nối MQTT TLS.
- Kiểm tra giới hạn cho ăn, chống lệnh trùng, khoảng cách tối thiểu giữa hai lần cho ăn và xử lý các trường hợp từ chối lệnh.
- Chỉ đưa các tệp mẫu không chứa thông tin thật lên GitHub; `secrets.h` và mật khẩu phải được giữ trên máy thành viên, không commit.

**Web frontend:**

- Phát triển và duy trì trang web trong `docs/`, gồm `index.html`, `style.css`, `app.js`, `manifest.json`, `sw.js` và thư mục `icons/`.
- Hoàn thiện đăng nhập giao diện, dashboard cảm biến, biểu đồ, trạng thái kết nối, sự kiện gần đây, chức năng cho ăn ngay và cài đặt ngưỡng/lịch qua MQTT.
- Dùng đúng WebSocket bảo mật của EMQX, topic và cấu trúc JSON đã thống nhất.
- Kiểm tra giao diện trên máy tính và điện thoại; thử các tình huống dữ liệu bình thường, cảnh báo và mất kết nối.
- Phối hợp với Thành viên 3 để chạy thử web với ESP32 giả; ghi rõ các kết quả đó là **MÔ PHỎNG**.

**Sản phẩm bàn giao:**

- Firmware trong `firmware/fish_tank/` và mã web trong `docs/`.
- Kết quả kiểm thử firmware T3.1–T3.11 và web T4.1–T4.9, kèm log/ảnh tương ứng.
- Hướng dẫn cấu hình bí mật cục bộ dựa trên tệp mẫu, không chứa mật khẩu thật.
- Các thay đổi code được commit lên nhánh làm việc của nhóm và tích hợp vào kho chung.

### Thành viên 3 – Tích hợp hệ thống và kiểm thử

**Mục tiêu:** Kiểm tra các thành phần hoạt động cùng nhau, quản lý bằng chứng kiểm thử và xác nhận hệ thống đáp ứng các kịch bản nghiệm thu.

**Công việc chính:**

- Làm đầu mối phối hợp giữa phần cứng, firmware, web và dịch vụ EMQX Cloud.
- Đối chiếu cấu hình EMQX với mục E1–E3: địa chỉ broker, cổng TLS, tài khoản, topic, QoS, retained message và quyền publish/subscribe.
- Theo dõi các bài kiểm thử MQTT Giai đoạn 1; lưu ý T1.8 bổ sung và kiểm tra quyền tài khoản `esp32` đã được nhóm bỏ qua có chủ đích, không ghi là đã đạt nếu chưa thử lại. Theo kế hoạch, T1.9 vẫn cần thực hiện.
- Phối hợp chạy các bài thử phần mềm bằng công cụ trong `tools/`, gồm `tools/fake-esp32/`, `tools/test-risk/`, `tools/test-sensors-alarm/` và `tools/test-store-feeder/`.
- Kiểm tra rằng kết quả từ ESP32 giả hoặc chương trình chạy trên máy tính được ghi rõ là **MÔ PHỎNG**, không thay thế kiểm thử trên thiết bị thật.
- Tổ chức kiểm thử ghép nối Giai đoạn 5 theo các kịch bản T5.1–T5.10: khởi động, thay đổi cảm biến, cho ăn, cảnh báo, mất mạng, offline, chạy liên tục, truy cập từ mạng di động, kiểm tra quyền và khởi động lại sau mất điện.
- Phối hợp triển khai Giai đoạn 6: GitHub Pages, HTTPS, xác minh chứng chỉ CA, rà soát quyền EMQX và kiểm tra kho GitHub không chứa bí mật.
- Quản lý kết quả kiểm thử trong `tests/phaseN/results.md` và ảnh/log trong `tests/phaseN/evidence/`.
- Tổng hợp các lỗi, phân công người xử lý và chạy kiểm thử hồi quy sau khi có sửa đổi.

**Sản phẩm bàn giao:**

- Bảng kết quả kiểm thử và bằng chứng có thể truy xuất trong `tests/`.
- Biên bản/kết quả tích hợp Giai đoạn 5 và kiểm tra triển khai, bảo mật Giai đoạn 6.
- Danh sách lỗi còn tồn tại, người xử lý và trạng thái khắc phục.
- Tài liệu kết quả demo và nghiệm thu trong `documentation/report/`.

## 3. Công việc cần phối hợp cả nhóm

| Công việc | Người phối hợp chính | Người phối hợp |
|---|---|---|
| Chốt sơ đồ đấu dây và kiểm tra an toàn nguồn | Thành viên 1 | Thành viên 2, 3 |
| Hiệu chuẩn cảm biến, servo và còi | Thành viên 1 | Thành viên 2 |
| Thống nhất topic, JSON, GPIO và ngưỡng theo mục E | Thành viên 2 | Thành viên 1, 3 |
| Kết nối firmware – EMQX – web | Thành viên 3 | Thành viên 1, 2 |
| Kiểm thử trên ESP32 thật | Thành viên 3 | Thành viên 1, 2 |
| Triển khai GitHub Pages và rà soát bảo mật | Thành viên 3 | Thành viên 2 |
| Báo cáo, slide và demo cuối | Cả nhóm | Cả nhóm |

## 4. Tiến độ và thứ tự ưu tiên


## 5. Quy tắc phối hợp và bàn giao

1. **Dùng kế hoạch làm nguồn thông số chuẩn:** mục E của `documentation/IoT-fish-tank-plan.docx` là nguồn sự thật duy nhất cho broker, tài khoản, topic, JSON, chân GPIO và ngưỡng mặc định. Khi cần thay đổi, nhóm thống nhất và cập nhật kế hoạch trước, sau đó mới sửa code.
2. **Không xác nhận khi chưa có bằng chứng:** mỗi bài thử cần có kết quả trong `tests/phaseN/results.md` và ảnh/log trong `tests/phaseN/evidence/`. Không coi mô phỏng là kiểm thử thiết bị thật.
3. **Bàn giao rõ ràng:** người hoàn thành một phần phải thông báo tệp đã sửa, cách chạy/thử, kết quả mong đợi, lỗi còn lại và đường dẫn bằng chứng cho các thành viên còn lại.
4. **Lưu tệp đúng vị trí:** code firmware ở `firmware/`, web ở `docs/`, tài liệu ở `documentation/`, kết quả và bằng chứng ở `tests/`, công cụ thử ở `tools/`.
5. **Bảo mật kho công khai:** không commit `secrets.h`, mật khẩu WiFi/EMQX, token Telegram hoặc API key. Không gửi các bí mật này trong cuộc trò chuyện. Trước khi commit, kiểm tra `git status` và nội dung tệp được đưa lên.
6. **Quy trình Git:** trước khi commit, chạy `git pull`, kiểm tra `git status`, chỉ thêm các tệp cần thiết, commit với nội dung theo mẫu `<phần>: <việc đã làm>`, sau đó push. Không dùng `git push --force` nếu nhóm chưa thống nhất.
7. **Cập nhật tài liệu:** khi thay đổi cấu hình hoặc quyết định dự án, cập nhật kế hoạch, README và tài liệu liên quan để các thành viên sử dụng cùng một thông tin.

---

**Vị trí lưu trong kho GitHub:** `documentation/team/phan-vai.md`  
**Đường dẫn trên máy theo kế hoạch:** `F:\IoT\smart-fish-tank\smart-fish-tank\documentation\team\phan-vai.md`
