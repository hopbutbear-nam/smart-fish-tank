# fake-esp32: Script giả lập ESP32

Script Python mô phỏng toàn bộ hành vi của firmware ESP32 (gửi telemetry, phản hồi `feed_done`/`feed_rejected`, nhận `config`, hỗ trợ `cmd/mute` và trường `muted`), dùng để kiểm thử giao diện web frontend khi chưa có hoặc không cắm phần cứng thật.

Mọi kết quả kiểm thử từ công cụ này bắt buộc phải ghi nhận là **MÔ PHỎNG** trong tài liệu và báo cáo.

## 1. Cài đặt môi trường

Cài đặt thư viện phụ thuộc bằng pip:
```bash
pip install -r tools/fake-esp32/requirements.txt
```

## 2. Thiết lập biến môi trường và chạy script

Không ghi mật khẩu vào mã nguồn. Đặt mật khẩu tài khoản MQTT `esp32` qua biến môi trường trước khi chạy:

**Trên Windows (PowerShell):**
```powershell
$env:MQTT_PASS = "mat-khau-esp32-cua-nhom"
python tools/fake-esp32/fake_esp32.py
```

**Trên Linux / macOS:**
```bash
export MQTT_PASS="mat-khau-esp32-cua-nhom"
python tools/fake-esp32/fake_esp32.py
```

## 3. Các lệnh điều khiển gõ tay trong khi chạy

Khi script đang chạy, bạn có thể gõ trực tiếp các lệnh sau vào terminal để thay đổi trạng thái mô phỏng:

- `normal`: Đưa mọi chỉ số về mức bình thường (26 °C, pH 7.0, độ đục 10%, mực nước 80%, risk 0).
- `hot`: Đặt nhiệt độ 30.5 °C (cảnh báo nhiệt độ cao, risk 1).
- `cold`: Đặt nhiệt độ 22.0 °C (cảnh báo nhiệt độ thấp, risk 1).
- `low_ph`: Đặt pH 5.8 (cảnh báo pH thấp, risk 1).
- `high_ph`: Đặt pH 8.2 (cảnh báo pH cao, risk 1).
- `turb`: Đặt độ đục 40% (cảnh báo nước đục, risk 1).
- `low_lvl`: Đặt mực nước 20% (cảnh báo nước thấp, risk 1).
- `demo2`: Đặt nhiều chỉ số vượt ngưỡng để kích hoạt cảnh báo rủi ro Mức 2.
- `demo3a`: Đặt nhiệt độ 33.5 °C để kích hoạt cảnh báo rủi ro nguy hiểm Mức 3.
- `demo3b`: Đặt nhiệt độ 17.5 °C để kích hoạt cảnh báo Mức 3 do quá lạnh.
- `set <thông số> <giá trị>`: Đặt thông số bất kỳ (ví dụ: `set temp 32`, `set ph 8.7`).
- `offline`: Giả lập sự cố mất kết nối mạng (bật Last Will).
- `online`: Kết nối lại bình thường.
- `quit` hoặc `exit`: Thoát chương trình.

## 4. Lưu ý quan trọng
- Script sử dụng Client ID `esp32-tank01` giống hệt ESP32 thật để kiểm tra kết nối với EMQX. Vì vậy, **chỉ chạy một bên tại một thời điểm** (chạy script mô phỏng thì tắt nguồn ESP32 thật và ngược lại) để tránh bị broker ngắt kết nối do trùng Client ID.
