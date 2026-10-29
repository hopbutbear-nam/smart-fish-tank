# firmware/

Mã Arduino cho ESP32. Môi trường: Arduino IDE 2.x, board **ESP32 Dev Module**, Serial 115200.

- `fish_tank/`: chương trình chính (Giai đoạn 3).
- `tests/`: các bài thử từng thiết bị (Giai đoạn 0 đến 2). Mỗi bài một thư mục, **tên thư mục trùng tên file .ino** (yêu cầu của Arduino IDE).
- `certs/`: chứng chỉ CA của EMQX (công khai, không phải bí mật).

## Quy tắc
- Không đưa `secrets.h` lên GitHub (đã nằm trong `.gitignore`). Dùng `secrets.example.h` làm mẫu.
- Không tự đổi tên topic, số chân GPIO, định dạng JSON: sửa mục E của kế hoạch trước, rồi mới sửa code.
- Ghi phiên bản gói esp32 (Boards Manager) vào `documentation/` mỗi khi biên dịch ra kết quả quan trọng.

## Thư viện cần cài
OneWire, DallasTemperature, PubSubClient, ArduinoJson (bản 7), ESP32Servo.
