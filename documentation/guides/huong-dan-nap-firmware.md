# Hướng dẫn nạp firmware cho ESP32

Tài liệu hướng dẫn từng bước cài đặt môi trường, cấu hình và nạp firmware cho bo mạch ESP32 của hệ thống IoT-fish-tank.

## 1. Chuẩn bị môi trường phần mềm

1. **Cài đặt Arduino IDE:**
   - Tải và cài đặt phiên bản Arduino IDE 2.x từ trang chủ [arduino.cc](https://www.arduino.cc/).
2. **Cài đặt gói bo mạch ESP32:**
   - Mở Arduino IDE, vào `File` → `Preferences`.
   - Tại ô *Additional boards manager URLs*, thêm đường dẫn:
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Vào menu `Tools` → `Board` → `Boards Manager...`, tìm từ khóa `esp32` (bởi Espressif Systems) và bấm **Install**.
3. **Cài đặt 5 thư viện cần thiết:**
   - Vào menu `Tools` → `Manage Libraries...` (hoặc `Ctrl+Shift+I`), tìm và cài đặt lần lượt:
     1. **ArduinoJson** (bắt buộc chọn phiên bản 7.x).
     2. **PubSubClient** (bởi Nick O'Leary).
     3. **OneWire** (bởi Paul Stoffregen / Jim Studt).
     4. **DallasTemperature** (bởi Miles Burton).
     5. **ESP32Servo** (bởi Kevin Harrington / John K. Bennett).

## 2. Chuẩn bị tệp cấu hình bí mật (secrets.h)

1. Đi đến thư mục mã nguồn firmware: `firmware/fish_tank/`.
2. Tạo một bản sao từ tệp mẫu `secrets.example.h` và đổi tên thành `secrets.h`.
3. Mở `secrets.h` trên máy tính cá nhân và điền thông tin mạng Wi-Fi cùng tài khoản EMQX Cloud của nhóm:
   - `WIFI_SSID`: Tên mạng Wi-Fi 2,4 GHz tại nơi đặt bể.
   - `WIFI_PASS`: Mật khẩu mạng Wi-Fi.
   - `MQTT_HOST`: Địa chỉ broker EMQX Serverless (xem mục E1 của kế hoạch).
   - `MQTT_PORT`: Cổng TLS bảo mật (8883).
   - `MQTT_USER`: Tên tài khoản MQTT cho ESP32 (`esp32`).
   - `MQTT_PASS`: Mật khẩu tài khoản MQTT `esp32`.
4. **Quy tắc an toàn bảo mật:** Tuyệt đối không commit tệp `secrets.h` lên GitHub. Tệp này đã được khai báo loại trừ trong `.gitignore`.

## 3. Kết nối phần cứng và cấu hình bo mạch

1. Cắm bo mạch ESP32 vào máy tính qua cáp USB Type-C.
2. Kiểm tra driver: Bo mạch sử dụng chip nạp CP2102. Nếu máy chưa nhận cổng COM, cài đặt CP210x USB to UART Bridge VCP Drivers.
3. Trong Arduino IDE, thiết lập cấu hình:
   - **Board:** `ESP32 Dev Module`
   - **Upload Speed:** `921600` (hoặc `115200` nếu nạp không ổn định)
   - **CPU Frequency:** `240MHz (WiFi/BT)`
   - **Flash Frequency:** `80MHz`
   - **Flash Mode:** `QIO`
   - **Port:** Chọn đúng cổng COM của ESP32 (ví dụ `COM3`, `COM5`).

## 4. Biên dịch và nạp chương trình

1. Mở tệp chính `firmware/fish_tank/fish_tank.ino` trong Arduino IDE. Toàn bộ các mô-đun liên quan (`config.h`, `sensors.*`, `risk.*`, `alarm.*`, `feeder.*`, `net.*`, `store.*`) sẽ tự động được tải kèm theo.
2. Bấm nút **Verify** (dấu tích ✔) để kiểm tra biên dịch mã nguồn.
   - *Lưu ý lỗi `ledc*`:* Code đã hỗ trợ tương thích cả ESP32 core 2.x và core 3.x. Nếu gặp cảnh báo hàm `ledcAttachPin`, kiểm tra lại phiên bản core và tham khảo mục G3 (T3.1) của kế hoạch.
3. Bấm nút **Upload** (mũi tên hướng sang phải ➔) để nạp firmware vào ESP32. Nếu IDE hiển thị `Connecting......`, nhấn giữ nhẹ nút **BOOT** trên bo mạch ESP32 cho đến khi quá trình ghi flash bắt đầu (`Writing at 0x...`).

## 5. Kiểm tra kết quả qua Serial Monitor

1. Mở **Serial Monitor** trong Arduino IDE (`Ctrl+Shift+M`), đặt tốc độ baud là `115200` và chọn `Both NL & CR`.
2. Bấm nút **EN / RST** trên bo mạch ESP32 để khởi động lại.
3. Chuỗi log khởi động chuẩn sẽ xuất hiện:
   ```text
   === fish_tank firmware v1.0.0 ===
   [NVS] Khoi tao bo nho thanh cong
   [WiFi] Dang ket noi toi WiFi...
   [WiFi] Da ket noi, IP: 192.168.x.x
   [NTP] Dong bo thoi gian thanh cong...
   [MQTT] Dang ket noi EMQX Cloud qua TLS 8883...
   [MQTT] Ket noi thanh cong! Status = online
   [Sensors] Bat dau doc chu ky cam bien
   ```
4. Nếu Serial in `status = online` và các bản tin telemetry định kỳ mỗi 10 giây, quá trình nạp firmware đã thành công hoàn toàn.
