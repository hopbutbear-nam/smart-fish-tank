# docs/ (web frontend, GitHub Pages)

GitHub Pages phục vụ thư mục này: Settings → Pages → Deploy from a branch → `main` / `/docs`.
Đường dẫn dự kiến (chưa bật, theo Q4): `https://hopbutbear-nam.github.io/smart-fish-tank/`

## Quy tắc
- Không ghi mật khẩu vào mã nguồn; người dùng nhập ở form đăng nhập (chỉ giữ trong sessionStorage).
- Thư viện mqtt.js 5.x và Chart.js 4.x nạp từ CDN, **ghim đúng một phiên bản** đã phát hành ít nhất 2 tuần và kiểm tra link tồn tại.
- Không tải tài nguyên từ nơi nào khác. Mọi chữ bằng Tiếng Việt.
- Kết nối: `wss://y132c53f.ala.asia-southeast1.emqxsl.com:8084/mqtt`
- Lịch cho ăn mặc định: `08:00`, `14:00`, `18:00` (được điền sẵn trong form Cài đặt của `app.js`).

## Màu sắc (bản 1.2)
- Mức cảnh báo 0 đến 3: xanh lá, vàng, cam, đỏ (mức 3 nhấp nháy). Mỗi thẻ tự tính mức riêng theo bảng mục E8; mức tổng `risk` hiện ở huy hiệu và thanh cảnh báo.
- Nhiệt độ: lạnh thì xanh dương, vừa thì xanh ngọc đến xanh lá, nóng thì vàng cam đỏ.
- pH: theo màu giấy chỉ thị (chua đỏ cam, trung tính xanh lá, kiềm xanh dương).
- Độ đục: trong thì xanh ngọc nhạt, đục dần thì xám, quá đục (có cảnh báo) thì nền xám đậm và chữ sáng.
- Mực nước: nền là thước đo nước dâng theo %, đổi sang hổ phách (mức 1) và đỏ (mức 2).
- Cài đặt và Đăng nhập: tông xanh dương nhạt. Hỗ trợ chế độ tối.

## Thanh "Tắt tiếng còi"
- Thanh điều khiển tắt tiếng còi chỉ xuất hiện khi hệ thống đang có cảnh báo (`risk > 0` hoặc còi đang kêu).
- Nút bấm gửi lệnh `cmd/mute` yêu cầu thiết bị tắt tiếng trong 30 phút. Nút tự ẩn khi hệ thống về mức 0 (bình thường).

## Sự kiện gần đây
- Thiết bị gửi `event` QoS 0, không retained: broker không giữ lịch sử.
- Các loại event được xử lý và hiển thị thông báo tiếng Việt:
  - `feed_done`: Đã cho ăn thành công
  - `feed_rejected`: Cho ăn bị từ chối (kèm lý do `too_soon`, `daily_limit`, `busy`, `no_time`)
  - `alarm`: Cảnh báo rủi ro tăng mức
  - `mute`: Còi đã được tắt tiếng 30 phút
  - `unmute`: Bật lại tiếng còi
  - `mute_rejected`: Từ chối tắt tiếng còi
  - `config_rejected`: Từ chối bản tin cấu hình không hợp lệ
- Web tự lưu vào localStorage (`ft_events`, tối đa 100 sự kiện, 7 ngày), loại trùng, dùng chung giữa các tab.
- Sự kiện xảy ra lúc trang đóng sẽ không có. Muốn lưu đầy đủ cần thêm lưu trữ phía thiết bị hoặc dịch vụ ngoài (xem Giai đoạn 7, lịch sử dài hạn).

## Kiểm thử
- Với ESP32 giả (`tools/fake-esp32`): gõ `demo2`, `demo3a`, `demo3b`, `set temp 32`, `set temp 35`, `set ph 8.7` và đối chiếu bảng ca thử mục E8 (T4.9, ghi là MÔ PHỎNG).

## Lưu ý
- Cảnh báo cùng mức chỉ hiện cách nhau tối thiểu 60/30/15 giây cho mức 1/2/3 (hằng số ALARM_GAP_MS trong app.js)
