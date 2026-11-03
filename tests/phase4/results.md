# Giai đoạn 4: Web frontend: kết quả kiểm thử

Cập nhật 03/11/2026 theo kế hoạch bản 1.5. Đổi thành Đạt chỉ khi có bằng chứng; chưa có thì ghi 'Đạt (theo lời nhóm, 03/11/2026)'.

| Mã | Nội dung | Kết quả | Ngày | Người làm | Bằng chứng (tên file trong `evidence/`) | Ghi chú |
| --- | --- | --- | --- | --- | --- | --- |
| T4.1 | Đăng nhập webuser: đã kết nối, thẻ hiện giá trị ngay (retained) | Đạt (MÔ PHỎNG); thiết bị thật: Đạt (theo lời nhóm, 03/11/2026) | | | T4.x_2026-10-30_mo-phong__frontend.png | Nhóm báo đạt trên thiết bị thật; ảnh MÔ PHỎNG |
| T4.2 | Sai mật khẩu: báo lỗi tiếng Việt, không treo | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Báo lỗi đúng tiếng Việt |
| T4.3 | Chạy 5 phút: số và biểu đồ cập nhật mỗi 10 giây, màu theo risk | Đạt (MÔ PHỎNG); thiết bị thật: Đạt (theo lời nhóm, 03/11/2026) | | | test-hot_esp32_fake.png, test-demo3a_esp32_fake.png | Biểu đồ cập nhật, đổi màu theo risk |
| T4.4 | Cho ăn ngay: servo quay, web hiện đã cho ăn; lần hai báo quá sớm | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Nút cho ăn gửi cmd/feed, nhận event feed_done |
| T4.5 | Rút nguồn ESP32: web báo offline trong 15-30 giây | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Báo offline theo Last Will |
| T4.6 | Đổi ngưỡng ở Cài đặt, ESP32 áp dụng, mở lại web vẫn thấy | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Publish config retained QoS 1 |
| T4.7 | Điện thoại bằng dữ liệu di động: dùng được mọi chức năng | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Dùng được trên điện thoại; GitHub Pages chưa bật (theo Q4) |
| T4.8 | Hai tab cùng lúc: cả hai nhận dữ liệu | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Đồng bộ dữ liệu đa tab |
| T4.9 | Web chạy với ESP32 giả (MÔ PHỎNG) | Đạt (MÔ PHỎNG) | 2026-10-30 | Nam | test-normal_esp32_fake.png, test-hot_esp32_fake.png, test-demo3a_esp32_fake.png | 4 ảnh mô phỏng trong evidence/; kết quả MÔ PHỎNG |

**Hoàn thành khi**: mọi bài trên đạt.
