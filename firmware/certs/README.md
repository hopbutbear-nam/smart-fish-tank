# firmware/certs/

Đặt file `emqx-ca.pem` (tải từ EMQX Cloud: Deployment Overview → MQTT Connection Information → CA Certificate).
Hết hạn 10/11/2031. Đây là chứng chỉ công khai nên được phép commit.
Dùng ở Giai đoạn 6: `net.setCACert(...)` thay cho `net.setInsecure()`.
