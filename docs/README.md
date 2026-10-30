# docs/ (web frontend, GitHub Pages)

GitHub Pages phục vụ thư mục này: Settings → Pages → Deploy from a branch → `main` / `/docs`.
Đường dẫn: `https://<tên-tài-khoản>.github.io/smart-fish-tank/`

## Quy tắc
- Không ghi mật khẩu vào mã nguồn; người dùng nhập ở form đăng nhập (chỉ giữ trong sessionStorage).
- Thư viện mqtt.js 5.x và Chart.js 4.x nạp từ CDN, **ghim đúng một phiên bản** đã phát hành ít nhất 2 tuần và kiểm tra link tồn tại.
- Không tải tài nguyên từ nơi nào khác. Mọi chữ bằng Tiếng Việt.
- Kết nối: `wss://y132c53f.ala.asia-southeast1.emqxsl.com:8084/mqtt`

## Màu sắc (bản 1.2)
- Mức cảnh báo 0 đến 3: xanh lá, vàng, cam, đỏ (mức 3 nhấp nháy). Mỗi thẻ tự tính mức riêng theo bảng mục E8; mức tổng `risk` hiện ở huy hiệu và thanh cảnh báo.
- Nhiệt độ: lạnh thì xanh dương, vừa thì xanh ngọc đến xanh lá, nóng thì vàng cam đỏ.
- pH: theo màu giấy chỉ thị (chua đỏ cam, trung tính xanh lá, kiềm xanh dương).
- Độ đục: trong thì xanh ngọc nhạt, đục dần thì xám, quá đục (có cảnh báo) thì nền xám đậm và chữ sáng.
- Mực nước: nền là thước đo nước dâng theo %, đổi sang hổ phách (mức 1) và đỏ (mức 2).
- Cài đặt và Đăng nhập: tông xanh dương nhạt. Hỗ trợ chế độ tối.

## Sự kiện gần đây
- Thiết bị gửi `event` QoS 0, không retained: broker không giữ lịch sử.
- Web tự lưu vào localStorage (`ft_events`, tối đa 100 sự kiện, 7 ngày), loại trùng, dùng chung giữa các tab.
- Sự kiện xảy ra lúc trang đóng sẽ không có. Muốn lưu đầy đủ cần thêm lưu trữ phía thiết bị hoặc dịch vụ ngoài (xem Giai đoạn 7, lịch sử dài hạn).

## Kiểm thử
- Với ESP32 giả (`tools/fake-esp32`): gõ `demo2`, `demo3a`, `demo3b`, `set temp 32`, `set temp 35`, `set ph 8.7` và đối chiếu bảng ca thử mục E8 (T4.9, ghi là MÔ PHỎNG).

## Lưu ý
- Cảnh báo cùng mức chỉ hiện cách nhau tối thiểu 60/30/15 giây cho mức 1/2/3 (hằng số ALARM_GAP_MS trong app.js)