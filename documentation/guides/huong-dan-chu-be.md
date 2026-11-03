# Hướng dẫn sử dụng cho chủ bể cá

Tài liệu hướng dẫn chủ bể cá cách theo dõi, điều khiển và chăm sóc bể thông qua hệ thống IoT-fish-tank.

## 1. Bật nguồn và chờ kết nối

1. Cắm nguồn adapter 5V (khuyến nghị 2A) vào bo mạch ESP32 của bể cá.
2. Thiết bị sẽ tự động khởi động và kết nối vào mạng Wi-Fi của nhà bạn.
3. Khi đèn tín hiệu báo ổn định (khoảng 10–15 giây sau khi bật nguồn), thiết bị sẽ đồng bộ thời gian thực từ máy chủ Internet (NTP) và kết nối tới máy chủ điện toán đám mây EMQX Cloud.
4. Ngay khi kết nối thành công, hệ thống bắt đầu đo định kỳ và gửi dữ liệu bể cá lên máy chủ mỗi 10 giây một lần.

## 2. Mở web và đăng nhập

1. Mở trình duyệt web trên điện thoại hoặc máy tính và truy cập vào địa chỉ giao diện quản lý bể cá (qua GitHub Pages hoặc tệp `docs/index.html`).
2. Nhập thông tin đăng nhập được cấp:
   - **Tên đăng nhập:** `webuser`
   - **Mật khẩu:** Mật khẩu tài khoản webuser đã thiết lập.
3. Bấm **Đăng nhập**. Trình duyệt sẽ lưu phiên đăng nhập an toàn trong tab hiện tại (`sessionStorage`). Khi kết nối thành công, thanh trạng thái sẽ chuyển sang màu xanh với dòng chữ **Đã kết nối**.

## 3. Ý nghĩa màu sắc và 4 mức cảnh báo

Hệ thống đánh giá tình trạng bể cá theo 4 mức rủi ro (`risk` từ 0 đến 3):

- **Mức 0 – Bình thường (Màu xanh lá):** Mọi chỉ số (nhiệt độ, pH, độ đục, mực nước) đều nằm trong giới hạn an toàn. Bể cá hoạt động tối ưu.
- **Màu vàng (Mức 1 – Cảnh báo nhẹ):** Có một chỉ số lệch nhẹ ra khỏi vùng an toàn hoặc phát hiện lỗi cảm biến. Cần theo dõi thêm.
- **Màu cam (Mức 2 – Cảnh báo vừa):** Có chỉ số lệch nhiều hoặc nhiều chỉ số cùng lúc ở mức 1. Cần can thiệp sớm (ví dụ thay nước, kiểm tra nhiệt độ).
- **Màu đỏ (Mức 3 – Nguy hiểm, nhấp nháy):** Nhiệt độ vượt ngưỡng nguy kịch (quá nóng trên 33 °C hoặc quá lạnh dưới 18 °C). Cần xử lý khẩn cấp để bảo vệ cá.

## 4. Cho ăn từ xa và các lý do bị từ chối

1. Tại giao diện điều khiển, bấm nút **Cho ăn ngay** để yêu cầu hệ thống cấp thức ăn cho cá (mỗi lần cho ăn servo quay 600 ms tương ứng một phần).
2. Khi lệnh được thực hiện, web sẽ nhận được thông báo **feed_done**.
3. **Các trường hợp lệnh bị từ chối:** Firmware tích hợp cơ chế bảo vệ cá chống bị cho ăn quá nhiều. Nếu lệnh bị từ chối, web sẽ hiển thị chính xác một trong các lý do sau:
   - `too_soon`: **quá sớm (phải chờ ít nhất 60 giây giữa hai lần)** – Khoảng cách an toàn giữa 2 lần cho ăn là tối thiểu 60 giây.
   - `daily_limit`: **đã đủ số lần trong 24 giờ qua** – Đã đạt giới hạn số lần cho ăn tối đa trong cửa sổ trượt 24 giờ (mặc định 4 lần mỗi 24 giờ; đang chờ chốt Q1).
   - `busy`: **đang cho ăn** – Cơ cấu servo đang quay một phần trước đó.
   - `no_time`: **thiết bị chưa đồng bộ giờ, thử lại sau ít phút** – ESP32 chưa lấy được giờ NTP nên tạm khóa cho ăn để tránh tính sai lịch.

## 5. Đổi ngưỡng cảnh báo và lịch cho ăn

1. Mở mục **Cài đặt** trên giao diện web.
2. Bạn có thể tùy chỉnh:
   - Ngưỡng nhiệt độ an toàn tối thiểu và tối đa (`temp_min`, `temp_max`).
   - Ngưỡng pH an toàn (`ph_min`, `ph_max`).
   - Lịch cho ăn tự động trong ngày: Hỗ trợ tối đa 3 mốc thời gian (lịch mặc định sẵn có là `08:00`, `14:00`, `18:00`).
3. Bấm **Lưu cài đặt**. Lệnh sẽ được gửi tới ESP32 qua cơ chế bảo đảm QoS 1 và lưu vào bộ nhớ flash NVS để giữ nguyên ngay cả khi mất điện.

## 6. Ý nghĩa tiếng còi và nút "Tắt tiếng còi 30 phút"

- **Mẫu âm thanh cảnh báo tại chỗ (tần số 1300 Hz):**
  - *Mức 0:* Còi hoàn toàn im lặng.
  - *Mức 1:* 1 tiếng ngắn nhỏ (duty cycle 15% hoặc 50% theo Q3) mỗi 30–60 giây.
  - *Mức 2:* 2 tiếng nhỏ mỗi 15–30 giây.
  - *Mức 3 (Nguy hiểm):* 3 tiếng kêu to nhất (duty cycle 50%) dồn dập mỗi 5 giây.
- **Tắt tiếng còi 30 phút:**
  - Khi có cảnh báo và còi đang kêu, trên web sẽ xuất hiện thanh màu cam có nút **Tắt tiếng còi (30 phút)**.
  - Bấm nút này sẽ gửi lệnh `cmd/mute` khiến còi im lặng trong 30 phút để tránh làm phiền trong lúc chủ bể kiểm tra, xử lý sự cố.
  - *Cơ chế an toàn:* Còi sẽ **tự động kích hoạt lại ngay lập tức** nếu mức độ nguy hiểm tăng lên (ví dụ từ mức 1 lên mức 2), hoặc khi hết thời gian 30 phút. Khi tình trạng bể trở về an toàn (mức 0), chế độ tắt tiếng sẽ tự giải trừ.

## 7. Vệ sinh và bảo quản đầu dò

- **Đầu dò pH:** Đây là linh kiện nhạy cảm nhất.
  - **Luôn giữ đầu dò pH ẩm:** Không bao giờ để đầu điện cực thủy tinh bị khô. Khi không nhúng trong bể cá, phải ngâm đầu dò trong nắp bảo vệ chứa dung dịch bảo quản 3M KCl.
  - **Hiệu chuẩn định kỳ:** Đầu dò pH có hiện tượng trôi nhẹ theo thời gian; nên dùng dung dịch chuẩn pH 6,86 và 4,01 để hiệu chuẩn lại sau khoảng 1 tuần hoạt động liên tục (theo mục G8).
- **Cảm biến mực nước:** Mạch đo đã được thiết kế chỉ cấp điện ngắt quãng khi đọc để chống điện phân ăn mòn. Định kỳ lau nhẹ các vạch kim loại để tránh đóng cặn bẩn.
- **Cảm biến độ đục:** Định kỳ lau sạch khe phát - thu quang học nếu bị rong rêu bám.

## 8. Xử lý sự cố nhanh

- **Web báo Offline:** Kiểm tra nguồn điện cấp cho ESP32 và đảm bảo mạng Wi-Fi tại nơi đặt bể đang hoạt động bình thường.
- **Còi kêu liên tục mức 3:** Kiểm tra ngay nhiệt độ nước bằng nhiệt kế rời; đưa biện pháp hạ nhiệt (hoặc sưởi) khẩn cấp nếu nhiệt độ ngoài khoảng 18–33 °C.
- **Lệnh cho ăn không phản hồi:** Kiểm tra xem thức ăn trong phễu có bị kẹt cơ học không; sau 10 giây cơ chế bảo vệ sẽ tự ngắt xung servo để tránh cháy động cơ. Tham khảo thêm tài liệu `xu-ly-su-co.md`.
