# smart-fish-tank: Hệ thống quản lý bể cá thông minh (IoT)

Bộ kit ESP32 đọc nhiệt độ, pH, độ đục, mực nước của bể cá; cho ăn bằng servo; còi cảnh báo tại chỗ;
gửi dữ liệu qua WiFi lên EMQX Cloud (MQTT) để chủ bể đi xa vẫn theo dõi và điều khiển qua web.

> Kho này là **CÔNG KHAI**. Không bao giờ đưa mật khẩu WiFi, mật khẩu EMQX, token Telegram vào kho.

## Kiến trúc
ESP32 (firmware tự viết) → EMQX Cloud Serverless (MQTT qua TLS) → web tĩnh (GitHub Pages). Không có server backend riêng.

## Cấu trúc kho
| Thư mục | Nội dung |
| --- | --- |
| `firmware/` | Mã Arduino cho ESP32: chương trình chính và các bài thử từng thiết bị |
| `docs/` | Trang web (GitHub Pages chỉ phục vụ từ `/` hoặc `/docs`) |
| `documentation/` | Kế hoạch, sơ đồ, bảng hiệu chuẩn, hướng dẫn, báo cáo |
| `tests/` | Kết quả kiểm thử T0 đến T6 và ảnh chụp bằng chứng |
| `tools/` | Dữ liệu mẫu MQTT, công cụ hỗ trợ thử |
| `extensions/` | Tính năng mở rộng (Giai đoạn 7) |

## Tiến độ theo giai đoạn
| GĐ | Tên | Trạng thái |
| --- | --- | --- |
| 0 | Chuẩn bị môi trường | Gần xong |
| 1 | Dựng cloud EMQX | Gần xong |
| 2 | Kiểm tra và hiệu chuẩn thiết bị | Đang làm |
| 3 | Firmware ESP32 | Chưa làm |
| 4 | Web frontend | Chưa làm |
| 5 | Ghép nối, thử nghiệm toàn hệ thống | Chưa làm |
| 6 | Đưa web lên mạng, bảo mật | Chưa làm |
| 7 | Tính năng mở rộng | Tùy chọn |
| 8 | Hoàn thiện và nghiệm thu | Chưa làm |

Thông số cố định (broker, topic, JSON, chân GPIO, ngưỡng) nằm ở **mục E** của
`documentation/IoT-fish-tank-plan.docx`: đó là nguồn sự thật duy nhất.

## Thành viên
Xem `documentation/team/phan-vai.md`.
