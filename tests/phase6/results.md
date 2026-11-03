# Giai đoạn 6: Đưa web lên mạng và bảo mật: kết quả kiểm thử

Cập nhật 03/11/2026 theo kế hoạch bản 1.5. Đổi thành Đạt chỉ khi có bằng chứng; chưa có thì ghi 'Đạt (theo lời nhóm, 03/11/2026)'.

| Mã | Nội dung | Kết quả | Ngày | Người làm | Bằng chứng (tên file trong `evidence/`) | Ghi chú |
| --- | --- | --- | --- | --- | --- | --- |
| T6.1 | Mở GitHub Pages trên máy tính và điện thoại qua HTTPS, đăng nhập được | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | GitHub Pages chưa bật (theo Q4); web chạy môi trường cục bộ |
| T6.2 | Nạp firmware dùng CA: kết nối EMQX khi xác minh chứng chỉ | Chưa | | | (chưa có tệp trong kho) | Hạn chế đã biết: net.cpp vẫn dùng setInsecure() |
| T6.3 | Chạy lại T1.4: vẫn bị chặn | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Whitelist ACL duy trì bảo vệ |
| T6.4 | Đăng nhập bằng mật khẩu cũ sau khi đổi: bị từ chối | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | Nhóm báo đã đổi mật khẩu ở H2 |
| T6.5 | Tìm password và tên người dùng trong toàn bộ kho: không có bí mật | Đạt (theo lời nhóm, 03/11/2026) | | | (chưa có tệp trong kho) | secrets.example.h đã giá trị giả (03/11/2026) |

**Hoàn thành khi**: mọi bài trên đạt.
