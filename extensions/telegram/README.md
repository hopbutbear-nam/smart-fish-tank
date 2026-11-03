# Thông báo Telegram

Trạng thái (03/11/2026): Chưa thực hiện (theo A14).

- Tạo bot qua @BotFather; lấy token và chat id.
- **Token và chat id là bí mật**: lưu trong NVS hoặc `secrets.h`, không commit. Lộ token thì `/revoke` ở BotFather.
- Giới hạn tần suất: mỗi sự cố nhắc tối đa 1 lần mỗi 30 phút.
