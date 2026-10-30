#pragma once
// config.h: mọi hằng số: chân GPIO, topic, giá trị mặc định, hằng số hiệu chuẩn.
// Nguồn sự thật: mục E của kế hoạch (E3 topic, E5 chân, E6 mặc định, E8 mức cảnh báo).
// KHÔNG đặt mật khẩu ở đây (WiFi và mật khẩu MQTT nằm trong secrets.h).
//
// File này CỐ Ý không #include Arduino.h để risk.cpp biên dịch được trên máy tính (kiểm thử T3.11).
#include <stdint.h>

// ---------------------------------------------------------------------------
// Định danh
// ---------------------------------------------------------------------------
#define FW_VERSION          "1.0.0"
#define TANK_ID             "tank01"          // đổi thì phải đổi đồng loạt ở web, ACL, tài liệu

// ---------------------------------------------------------------------------
// MQTT (E1, E2, E3)
// ---------------------------------------------------------------------------
#define MQTT_HOST           "y132c53f.ala.asia-southeast1.emqxsl.com"
#define MQTT_PORT           8883               // MQTT qua TLS
#define MQTT_USER           "esp32"
#define MQTT_CLIENT_ID      "esp32-tank01"     // dùng chung với fake_esp32.py: chỉ chạy một bên
#define MQTT_KEEPALIVE_S    30
#define MQTT_BUFFER_SIZE    512                // mqtt.setBufferSize(...)

#define TOPIC_BASE          "fishtank/" TANK_ID "/"
#define TOPIC_TELEMETRY     TOPIC_BASE "telemetry"   // QoS 0, retained
#define TOPIC_STATUS        TOPIC_BASE "status"      // online / offline, retained (Last Will QoS 1)
#define TOPIC_EVENT         TOPIC_BASE "event"       // QoS 0
#define TOPIC_CMD_FEED      TOPIC_BASE "cmd/feed"    // QoS 1 (ESP32 subscribe)
#define TOPIC_CMD_ALL       TOPIC_BASE "cmd/#"
#define TOPIC_CONFIG        TOPIC_BASE "config"      // QoS 1, retained (ESP32 subscribe)

// ---------------------------------------------------------------------------
// Chân GPIO (E5)
// ---------------------------------------------------------------------------
#define PIN_DS18B20         32
#define PIN_PH              34   // chỉ là chân vào; qua cầu chia áp 1/2
#define PIN_TURB            33   // qua cầu chia áp 1/2
#define PIN_LEVEL           35   // chân S của cảm biến mực nước
#define PIN_LEVEL_PWR       27   // chân + của cảm biến mực nước: chỉ bật khoảng 20 ms khi đo
#define PIN_BUZZER          22
#define PIN_SERVO           25

// ---------------------------------------------------------------------------
// Chu kỳ (E6)
// ---------------------------------------------------------------------------
#define TEMP_PERIOD_MS          2000     // đọc nhiệt độ
#define SLOW_PERIOD_MS          5000     // đọc pH, độ đục, mực nước
#define RISK_PERIOD_MS          5000     // chấm điểm rủi ro (mỗi chu kỳ đọc chậm, để 3 lần đọc = 15 giây)
#define TELEMETRY_PERIOD_MS     10000    // gửi telemetry (cũng gửi ngay khi risk đổi)
#define ADC_SAMPLES             32       // trung bình 16 đến 32 mẫu mỗi lần đọc

// ---------------------------------------------------------------------------
// Ngưỡng mặc định (E6, E8): dùng khi chưa có config trong NVS
// ---------------------------------------------------------------------------
#define DEF_TEMP_MIN            23.0f
#define DEF_TEMP_MAX            29.0f
#define DEF_PH_MIN              6.0f
#define DEF_PH_MAX              8.0f
#define DEF_TURB_MAX_PCT        30.0f
#define DEF_LEVEL_MIN_PCT       30.0f
#define DEF_LEVEL_DANGER_PCT    15.0f
#define DEF_BUZZER_ON           true
#define DEF_MUTE_MIN            30

// Biên nguy hiểm: cố định, cũng là giới hạn khi nhận config (web và firmware đều chặn)
#define TEMP_DANGER_MIN         18.0f    // dưới mức này: nhiệt độ mức 3
#define TEMP_DANGER_MAX         33.0f    // trên mức này: nhiệt độ mức 3
#define PH_DANGER_MIN           5.0f     // dưới mức này: pH mức 2 (pH không có mức 3)
#define PH_DANGER_MAX           8.5f

// Mức 2 = ngưỡng mức 1 cộng thêm (E8): đi theo ngưỡng mức 1 khi web đổi ngưỡng
#define TEMP_L2_EXTRA           2.0f     // °C
#define TURB_L2_EXTRA           15.0f    // điểm %

#define SENSOR_FAULT_LEVEL      1        // cảm biến nhiệt độ hoặc pH lỗi: mức 1 (nhóm chốt)

// ---------------------------------------------------------------------------
// Chống nhấp nháy (D3, ĐỀ XUẤT phương án A)
// ---------------------------------------------------------------------------
#define RISK_RAISE_READS        3        // nâng mức khi điều kiện đúng 3 lần đánh giá liên tiếp
#define HYST_TEMP               0.2f     // hạ mức khi quay lại trong ngưỡng một khoảng nhỏ
#define HYST_PH                 0.1f
#define HYST_PCT                2.0f

// ---------------------------------------------------------------------------
// Đọc cảm biến
// ---------------------------------------------------------------------------
#define DS18B20_FAULT_C         (-127.0f)  // thư viện trả về khi mất cảm biến
#define DS18B20_POWERON_C       85.0f      // giá trị mặc định của chip ngay sau khi cấp nguồn (D4)
#define ADC_DIVIDER             2.0f       // cầu chia áp 4,7k + 4,7k: nhân 2 để ra điện áp thật của module

// --- Bổ sung cho sensors (bước 5) ---
#define TEMP_PLAUSIBLE_MIN      (-55.0f)   // giới hạn đo của DS18B20; ngoài dải coi là lỗi
#define TEMP_PLAUSIBLE_MAX      125.0f
#define TEMP_CONVERT_MS         800        // DS18B20 12 bit cần tối đa 750 ms để chuyển đổi
#define ADC_SAMPLE_GAP_MS       3          // khoảng cách giữa hai mẫu ADC của pH và độ đục
#define ADC_V_MIN_VALID         0.05f      // điện áp module (V) ngoài dải này = dây đứt hoặc chập
#define ADC_V_MAX_VALID         5.2f
#define PH_PLAUSIBLE_MIN        0.0f       // pH tính ra ngoài 0 đến 14 = lỗi cảm biến
#define PH_PLAUSIBLE_MAX        14.0f
#define LEVEL_SAMPLES           8          // số mẫu đọc mực nước mỗi lần (giữ GPIO27 bật ngắn)
#define LEVEL_SAMPLE_GAP_MS     1

// ---------------------------------------------------------------------------
// Hằng số hiệu chuẩn (bảng hiệu chuẩn Giai đoạn 2)
// !!! GIÁ TRỊ TẠM, CHƯA ĐO: Giai đoạn 2 chưa làm. Đo xong phải thay đủ các dòng dưới đây
// !!! rồi đặt CALIBRATION_DONE = 1. Số pH, độ đục, mực nước trước đó CHỈ để thử luồng dữ liệu.
// ---------------------------------------------------------------------------
#define CALIBRATION_DONE        0
#define PH_V686                 2.50f      // V của module pH trong dung dịch 6,86 (V)
#define PH_V401                 3.00f      // V của module pH trong dung dịch 4,01 (V)
#define TURB_V_CLEAR            4.00f      // V của module độ đục trong nước sạch (V)
#define TURB_V_DIRTY            2.50f      // V trong nước rất đục (V)
#define LEVEL_RAW_DRY           0          // giá trị ADC khi khô
#define LEVEL_RAW_FULL          2500       // giá trị ADC khi ngập tới mức đầy
#define LEVEL_PWR_SETTLE_MS     20         // chờ sau khi bật GPIO27 rồi mới đọc

// ---------------------------------------------------------------------------
// Servo cho ăn (E6)
// ---------------------------------------------------------------------------
#define SERVO_STOP_US           1500       // điểm dừng: chỉnh ở Giai đoạn 2 (T2.5)
#define SERVO_FEED_US           1300       // CHIỀU QUAY chưa xác định (1300 hay 1700): xác nhận ở T2.5/T2.6
#define SERVO_MIN_US            500
#define SERVO_MAX_US            2400
#define FEED_MS_DEFAULT         600        // ms cho một phần (hiệu chuẩn lại ở T2.6)
#define FEED_MS_MIN             100
#define FEED_MS_MAX             3000       // giới hạn an toàn khi nhận config
#define MAX_PORTIONS            2          // D7: tối đa 2 phần mỗi lệnh
#define FEED_HARD_CAP_MS        10000      // D7: mốc cứng, quá thì luôn đưa servo về SERVO_STOP_US
#define FEED_MIN_INTERVAL_MS    60000UL    // tối thiểu 60 giây giữa hai lần
#define FEED_MAX_PER_24H        4          // D5: cửa sổ trượt 24 giờ, dùng chung cho lịch và lệnh từ xa
#define FEED_WINDOW_S           86400UL
#define FEED_MAX_TIMES          3          // D6: tối đa 3 mốc trong lịch (chừa 1 lần cho "Cho ăn ngay")
#define FEED_SCHEDULE_GRACE_S   300UL      // D6: mỗi mốc chạy trong cửa sổ khoảng 5 phút sau giờ đặt

// ---------------------------------------------------------------------------
// Còi (E6, D9)
// ---------------------------------------------------------------------------
#define BUZZ_FREQ_HZ            1300
#define BUZZ_DUTY_QUIET_PCT     15         // tiếng nhỏ (mức 1, 2): chọn bằng tai ở T2.7 (khoảng 10 đến 20)
#define BUZZ_DUTY_LOUD_PCT      50         // tiếng to nhất (mức 3)
#define BUZZ_BEEP_MS            200
#define BUZZ_L1_PERIOD_MS       60000UL    // mức 1: 1 tiếng ngắn mỗi 60 giây
#define BUZZ_L2_PERIOD_MS       30000UL    // mức 2: 2 tiếng mỗi 30 giây (cách nhau BUZZ_BEEP_MS)
                                           // mức 3: kêu to nhất liên tục
#define BUZZ_LEDC_CHANNEL       4          // chỉ dùng cho core 2.x (core 3.x tự cấp kênh); tránh kênh 0 của servo
#define BUZZ_LEDC_BITS          8          // độ phân giải PWM
#define BUZZ_ACTIVE_LOW         0          // đặt 1 nếu còi kêu liên tục khi chưa phát (đảo logic, mục E5)

// ---------------------------------------------------------------------------
// Mạng và độ bền (E6, G Giai đoạn 3)
// ---------------------------------------------------------------------------
#define NTP_SERVER              "pool.ntp.org"
#define TZ_OFFSET_S             (7 * 3600) // UTC+7
#define RECONNECT_MIN_MS        1000       // chờ tăng dần 1, 2, 4 ... giây
#define RECONNECT_MAX_MS        30000      // tối đa 30 giây
#define REBOOT_IF_NO_MQTT_MS    (10UL * 60UL * 1000UL)  // mất MQTT quá 10 phút thì tự khởi động lại
#define WDT_TIMEOUT_S           30
