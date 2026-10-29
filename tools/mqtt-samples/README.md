# mqtt-samples/

Dùng MQTTX, kết nối tài khoản `esp32` (client ID `esp32-tank01`) qua mqtts 8883, rồi publish:

| File | Topic | QoS | Retain |
| --- | --- | --- | --- |
| `telemetry-*.json` | fishtank/tank01/telemetry | 0 | bật |
| `event-*.json` | fishtank/tank01/event | 0 | tắt |
| (chữ) `online` / `offline` | fishtank/tank01/status | 1 | bật |

Last Will cấu hình trong MQTTX: topic status, QoS 1, Retain bật, payload `offline`.
Với `event-feed-*`, thay `id` bằng đúng `id` mà web vừa gửi trong `cmd/feed`.
Dọn dẹp sau khi thử: publish payload rỗng, Retain bật, vào `telemetry` và `status`.
