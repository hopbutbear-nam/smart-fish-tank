# tests/

Mỗi giai đoạn một thư mục `phaseN/` gồm `results.md` (bảng kết quả) và `evidence/` (ảnh chụp, log Serial).

Quy ước tên bằng chứng: `T<mã>_<ngày>_<mô tả>.png` hoặc `.txt`, ví dụ `T1.5_2026-10-29_wss.png`.
Ảnh từ ESP32 giả hoặc máy tính đặt tên: `T<mã>_<ngày>_mo-phong_<mô tả>.png`.
Che mật khẩu trước khi chụp. Không đưa file chứa bí mật lên kho.

## Quy ước trạng thái (bản 1.5)

| Cách ghi | Dùng khi | Điều kiện |
|---|---|---|
| Đạt + tên tệp trong evidence/ | Có ảnh hoặc log thật nằm trong kho | Agent kiểm bằng ls rằng tệp tồn tại; không có thì không dùng cách ghi này |
| Đạt (theo lời nhóm, 03/11/2026) | Nhóm báo đạt nhưng kho chưa có bằng chứng | Cột "Bằng chứng" ghi (chưa có tệp trong kho) |
| Đạt (MÔ PHỎNG) | Chạy trên máy tính hoặc ESP32 giả | Không thay cho bài thử trên thiết bị thật |
| Một phần + ghi rõ phần còn thiếu | Nhóm báo chỉ làm một phần | Ghi đúng phần nhóm nói |
| Chưa hoặc ☐ | Nhóm không xác nhận, hoặc ô Phụ lục A để trống mà không có nguồn khác | Không suy ra "đạt" từ việc bài sau đã chạy |
| Nhóm điền | Ô cần số liệu (ngày, người, số đo) mà Phụ lục A để trống | Không ước lượng, không lấy ngày commit làm ngày đo trừ khi Phụ lục A cho phép |
