# Quy ước làm việc với kho

## Trước mỗi lần commit
- Không có `secrets.h`, mật khẩu, token, API key trong thay đổi.
- File `.ino` đã xóa mật khẩu WiFi/EMQX điền tạm (ví dụ `test4_emqx.ino`).
- Nếu lỡ commit bí mật: **đổi mật khẩu ngay** (lịch sử vẫn lưu), rồi mới xóa file.

## Tải lên bằng giao diện web
Add file → Upload files → kéo thư mục vào → ghi nội dung commit → Commit changes.

## Cách ghi nội dung commit
`<phần>: <việc đã làm>`, ví dụ:
- `firmware: thêm đọc DS18B20`
- `web: thêm nút cho ăn`
- `tests: kết quả T1.5`
- `docs: sơ đồ đấu dây`

## Ảnh bằng chứng
Đặt vào `tests/phaseN/evidence/`, tên: `T1.5_2026-10-29_wss.png`. Che mật khẩu trước khi chụp.
