# firmware/certs/

Đặt file `emqx-ca.pem` (tải từ EMQX Cloud: Deployment Overview → MQTT Connection Information → CA Certificate).
Hết hạn: 10/11/2031. Đây là chứng chỉ công khai nên được phép commit.

Trạng thái hiện tại (03/11/2026, theo Q4): Firmware ở Giai đoạn 3 đang dùng cấu hình TLS tạm `setInsecure()`; chưa chuyển sang dùng CA `net.setCACert(...)` tại thời điểm này.
