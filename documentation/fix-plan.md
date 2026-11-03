# fix-plan.md: kế hoạch sửa lỗi và tối ưu (nguồn sự thật duy nhất cho Agent antiGravity)

> **Trạng thái:** đã áp dụng ở bản 1.4 (các commit từ 8ae14d2 đến ae5c064, 01/11/2026). Giữ làm lịch sử, không áp dụng lại.

| Mục | Giá trị |
|---|---|
| Dự án | Hệ thống bể cá thông minh (IoT-fish-tank), kho `smart-fish-tank` |
| Tệp này | `documentation/fix-plan.md` |
| Lập ngày | 01/11/2026 |
| Căn cứ | Kế hoạch `documentation/IoT-fish-tank-plan.docx` bản 1.3 (mục E, F5, H2), mã nguồn tại commit `77bffa5`, và các quyết định của nhóm trong cuộc trao đổi đối chiếu |
| Đích của kế hoạch | Nâng kế hoạch lên bản 1.4 và đưa code, web, ESP32 giả, bộ ca thử về đúng các quyết định của nhóm |
| Cách kiểm chứng đã làm | Mọi bản vá trong tệp này đã được áp tuần tự lên một bản sao sạch của kho và chạy thử: `test_risk` 72 ca, `test_sensors_alarm` 101 ca, `test_store_feeder` 89 ca, tất cả 0 sai; `node --check` và `py_compile` đạt; kịch bản ESP32 giả ngoại tuyến 15 kiểm tra đạt; `net.cpp` và `fish_tank.ino` đạt kiểm tra cú pháp với tệp giả của thư viện Arduino |
| Chưa kiểm chứng | Biên dịch và nạp thật bằng Arduino IDE (T3.1), tiếng còi thật (T3.9), mạng thật, servo thật. Các kết quả ở trên đều là **MÔ PHỎNG** theo quy ước mục A5 của kế hoạch |

> **Đọc trước:** Agent hãy đọc hết tệp này rồi mới sửa. Thực hiện đúng thứ tự Phase 0, 1, 2, 3, 4, 5. Một Phase chỉ xong khi mục "Kiểm tra sau Phase" của nó đạt. Chưa đạt thì dừng và báo nhóm, không sang Phase sau.

---

## 1. Tổng quan mục tiêu và quy tắc thực thi

### 1.1 Mục tiêu

Đưa bộ mã nguồn (firmware, web, công cụ thử) và tài liệu kế hoạch về đúng các quyết định đã thống nhất với nhóm, gồm bảy việc #1 đến #7 và hai thay đổi về hạ mức cảnh báo và còi (D3, D9, #9). Việc #8 (mực nước đọc 8 mẫu) nhóm đã quyết định bỏ qua.

### 1.2 Các quyết định đã chốt (đây là yêu cầu, không phải gợi ý)

| Mã | Quyết định | Giá trị cụ thể |
|---|---|---|
| Ngưỡng | Giữ cách tính "lệch" từ giá trị chuẩn 26 °C và pH 7,0 (khoảng 25 đến 27 chỉ để vẽ vùng xanh trên web) | Mức 1: nhiệt độ ngoài 23 đến 29 °C, pH ngoài 6,0 đến 8,0, độ đục trên 30%, mực nước dưới 30%. Mức 2: nhiệt độ ngoài 21 đến 31 °C, pH ngoài 5,0 đến 8,5, độ đục trên 45%, mực nước dưới 15%. Mức 3 (chỉ nhiệt độ): ngoài 18 đến 33 °C. Không đổi |
| D3 nâng | Nâng cảnh báo khi điều kiện đúng 3 lần đánh giá liên tiếp (mỗi lần cách nhau 5 giây) | Không đổi so với code |
| D3 hạ | Hạ khi quay lại sâu trong ngưỡng | **0,5 °C, 0,3 pH, 5 điểm %** (trước đây 0,2 / 0,1 / 2). So sánh nghiêm ngặt: nhiệt độ cao nâng khi > 29,0 và hạ khi ≤ 28,5; pH cao nâng khi > 8,0 và hạ khi ≤ 7,7; độ đục nâng khi > 30 và hạ khi ≤ 25 |
| #9 (hướng A) | Thêm khoảng đệm cho biên nguy hiểm khi hạ mức | Nhiệt độ hạ từ mức 3 xuống mức 2 khi ≤ 33,0 − 0,5 = **32,5 °C** (hoặc ≥ 18,0 + 0,5 = **18,5 °C**). pH hạ từ mức 2 xuống mức 1 khi ≤ 8,5 − 0,3 = **8,2** (hoặc ≥ 5,0 + 0,3 = **5,3**) |
| D4, D5 | Giữ nguyên | Cảm biến lỗi mức 1, lỗi nhiệt độ khi đọc −127 hoặc đúng 85,0 °C; hạn mức cho ăn 4 lần trong cửa sổ trượt 24 giờ |
| D6, D7 | Đồng ý | Tối đa 3 mốc lịch, cửa sổ 5 phút, lỡ giờ thì bỏ; tối đa 2 phần mỗi lệnh, mốc cứng servo 10 giây |
| D9 còi | Đổi mức 3 | Tần số 1300 Hz. Mức 0 im. Mức 1: 1 tiếng ngắn nhỏ (duty 15%) mỗi 60 giây. Mức 2: 2 tiếng nhỏ (duty 15%) mỗi 30 giây. **Mức 3: to nhất (duty 50%), 3 tiếng liền nhau mỗi 5 giây** |
| Tắt tiếng | Đồng ý | Im 30 phút (`mute_min`). Tự hủy khi risk tăng thêm hoặc khi risk về 0. Nếu risk giảm (ví dụ 2 xuống 1) vẫn giữ tắt tiếng. Áp dụng cả ở mức 3 |
| #1 | Lịch cho ăn mặc định | `08:00`, `14:00`, `18:00` |
| #2, #3 | Cờ lỗi cảm biến độ đục và mực nước | `sensor_turb`, `sensor_level`, mức 1; `FL_COUNT` thành 10; mặt nạ cờ 16 bit; giá trị đọc lỗi thì telemetry gửi `null` |
| #4, #5 | Lệnh `cmd/mute` và nút trên web | Topic `fishtank/tank01/cmd/mute`, payload `{"action":"mute"}` hoặc `{"action":"unmute"}`; web có nút 🔕 và chữ cho event `mute`, `unmute`, `mute_rejected` (và `config_rejected`) |
| #6 | Khởi động lại khi mất mạng | Không khởi động lại mỗi 10 phút khi mất WiFi nhà |
| #7 | Ghi NVS | Chỉ ghi khi cấu hình nhận từ MQTT thực sự khác cấu hình đang dùng |
| #8 | Bỏ qua | Không sửa |

### 1.3 Quy tắc thực thi và an toàn khi chỉnh sửa

1. **Tài liệu trước, code sau** (quy tắc A1 của kế hoạch): làm Phase 0 trước. Không đổi tên topic, số chân GPIO hay định dạng JSON ngoài những thay đổi liệt kê trong tệp này.
2. **Mỗi Phase là một commit riêng** (thông điệp gợi ý ở cuối mỗi Phase). Không `git push --force`. Không commit nếu kiểm tra sau Phase chưa đạt.
3. **Không bao giờ** đọc, in, ghi lại hay commit mật khẩu (WiFi, EMQX). Không mở `firmware/fish_tank/secrets.h`. Không `git add` các tệp `secrets.h`, `*.exe`, `*.env`; không stage `docs/icons/icon.svg` (đã bị sửa từ trước, không thuộc kế hoạch này).
4. **Bộ ca thử là chuẩn.** Quy tắc đầu `test_risk.cpp` vẫn đúng: ca sai thì sửa code theo bảng, không sửa bảng cho khớp code. Các thay đổi bộ ca thử trong tệp này là theo quyết định của nhóm (đổi khoảng đệm, thêm cờ, còi mức 3), đã có kiểm chứng.
5. **Định vị bằng neo văn bản, không dựa vào số dòng.** Số dòng chỉ để tham khảo. Nếu `patch` không áp được hoặc một neo không còn khớp: **dừng, báo nhóm**, không ép, không đoán.
6. **Không khẳng định điều chưa kiểm chứng** (mục A2): mọi kết quả chạy trên máy tính hoặc ESP32 giả ghi là **MÔ PHỎNG**. Không điền kết quả vào `tests/phase*/results.md` hay viết "Đạt" thay nhóm; chỉ nhóm mới chụp ảnh và ghi bằng chứng trên thiết bị thật.
7. **Quy ước văn phong khi sửa docx** (khớp tài liệu hiện có): tiếng Việt, số thập phân dùng dấu phẩy (0,5 °C), dùng chữ "đến" thay cho dấu gạch ngang giữa hai số, ký hiệu ✔ ⚠ ☐ **CẦN XÁC NHẬN** **MÔ PHỎNG** theo mục A5, chỉ sửa đúng chỗ cần sửa, giữ nguyên định dạng và bảng, không dựng lại tài liệu từ markdown.
8. **Gặp điều chưa rõ trong quyết định** thì hỏi nhóm (mỗi lần tối đa một câu hỏi), không tự quyết.

### 1.4 Cách áp dụng các bản vá trong tệp này

Mỗi khối ```` ```diff ```` bên dưới là một phần của bản vá dạng `diff -u` (có tiền tố `a/` và `b/`), áp được bằng `patch -p1` hoặc `git apply` từ thư mục gốc của kho. Các Phase **phụ thuộc nhau theo thứ tự**: số dòng trong Phase sau tính trên tệp đã áp xong Phase trước.

Cách nhanh (kịch bản trích xuất, chạy từ thư mục gốc của kho; tạo thư mục `fix-plan-patches/`, không commit thư mục này):

```python
import re, sys, pathlib
md = pathlib.Path("documentation/fix-plan.md").read_text(encoding="utf-8")
phase, blocks = None, {}
for line_group in re.split(r"(?m)^(?=## )", md):
    m = re.match(r"## Phase (\d)", line_group)
    if not m:
        continue
    phase = int(m.group(1))
    for body in re.findall(r"(?s)```diff\n(.*?)\n```", line_group):
        blocks.setdefault(phase, []).append(body + "\n")
out = pathlib.Path("fix-plan-patches"); out.mkdir(exist_ok=True)
for n in sorted(blocks):
    (out / f"phase{n}.patch").write_text("".join(blocks[n]), encoding="utf-8", newline="\n")
    print(f"phase{n}.patch: {len(blocks[n])} tệp")
```

Áp dụng từng Phase (kiểm tra trước khi áp thật):

```bash
# Linux, macOS, Git Bash
patch -p1 --dry-run < fix-plan-patches/phase1.patch && patch -p1 --no-backup-if-mismatch < fix-plan-patches/phase1.patch
# hoặc: git apply --check fix-plan-patches/phase1.patch && git apply fix-plan-patches/phase1.patch
```

```powershell
# Windows PowerShell (nếu có git)
git apply --check fix-plan-patches/phase1.patch; if ($?) { git apply fix-plan-patches/phase1.patch }
```

Nếu không dùng được `patch` hay `git apply`, áp tay từng khối: mỗi tệp có bảng "Vị trí tác động, Thao tác" và khối diff cho thấy chính xác dòng bị **XÓA** (bắt đầu bằng `-`) và dòng được **THÊM** (bắt đầu bằng `+`). Dòng không có dấu là ngữ cảnh để định vị.

### 1.5 Giả định cần nhóm xác nhận (Agent ghi vào H2 của docx, đừng tự coi là đã chốt)

| # | Giả định đã dùng | Lý do |
|---|---|---|
| 1 | Mực nước đối xứng với độ đục: nâng khi < 30%, hạ khi ≥ 35%; mức 2 của mực nước nâng khi < 15%, hạ về mức 1 khi ≥ 20% | Nhóm chỉ nêu ví dụ cho độ đục (nâng > 30, hạ ≤ 25) |
| 2 | `LEVEL_RAW_SHORT = 4090` và `LEVEL_SPREAD_MAX = 300` là **giá trị tạm** để phát hiện lỗi cảm biến mực nước | Chưa có số đo thật; chốt khi hiệu chuẩn ở T2.2 và T2.3. Phải luôn lớn hơn `LEVEL_RAW_FULL` |
| 3 | Tắt tiếng ở mức 3 cũng dùng đủ `mute_min` (30 phút) | Nhóm xác nhận đồng ý đề xuất tắt tiếng, kể cả mức 3 |
| 4 | Không tự khởi động lại khi `risk > 0`; mất cả WiFi thì chờ 30 phút | Mở rộng yêu cầu #6 để không ngắt còi lúc đang cảnh báo |
| 5 | Khoảng đệm tự giảm khi ngưỡng cấu hình quá hẹp (không quá 1/4 độ rộng vùng an toàn; độ đục không quá một nửa ngưỡng) | Tránh tình huống cờ không bao giờ tắt; với cấu hình mặc định không có tác dụng |
| 6 | Trường `muted` trong telemetry là **tùy chọn** (web coi thiếu là `false`) | Tương thích với firmware cũ và dữ liệu retained cũ |

### 1.6 Ngoài phạm vi (KHÔNG làm)

- Không sửa khối **Cảnh báo bảo mật** và dòng ghi chú liền sau nó ở cuối mục F5 của docx (nhóm tự xử lý).
- Không đổi `setInsecure()` (làm ở Giai đoạn 6, T6.2), không đổi ACL hay tài khoản trên EMQX.
- Không sửa `tests/phase*/results.md`, không thêm ảnh bằng chứng.
- Không đụng #8 (mực nước đọc 8 mẫu).
- Không đổi hằng số hiệu chuẩn (`CALIBRATION_DONE`, `SERVO_*`, `PH_*`, `TURB_*`) ngoài hai hằng số mới liệt kê.

---

## 2. Danh sách các tệp cần chỉnh sửa

| Tệp | Phase | Nội dung chính |
|---|---|---|
| `documentation/IoT-fish-tank-plan.docx` | 0 | Nâng lên bản 1.4: E3, E4, E6, E8, E10, E11, F5, G, H2, H5 |
| `firmware/fish_tank/config.h` | 1, 2, 3, 4 | Khoảng đệm, hằng số lỗi cảm biến, lịch mặc định, còi mức 3, topic mute, hằng số khởi động lại |
| `firmware/fish_tank/risk.h` | 1 | Hai cờ mới, 4 biên nguy hiểm, `turb_ok`, `level_ok` |
| `firmware/fish_tank/risk.cpp` | 1 | `FLAG_NAMES`, `riskTighten()`, `riskComputeLevels()` |
| `firmware/fish_tank/sensors.h` | 1 | `SensorValues`, khai báo `sensorsLevelOk()` |
| `firmware/fish_tank/sensors.cpp` | 1 | `sensorsLevelOk()`, `finishChannel()`, `sensorsToRiskInput()` |
| `firmware/fish_tank/store.h` | 4 | Khai báo `configEqual()` |
| `firmware/fish_tank/store.cpp` | 1, 2, 4 | `configThresholds()`, `configDefaults()`, `configEqual()` |
| `firmware/fish_tank/alarm.h` | 3 | Chú thích mức 3 |
| `firmware/fish_tank/alarm.cpp` | 3 | Mẫu còi mức 3 |
| `firmware/fish_tank/net.h` | 3 | Khai báo mute, thêm tham số `muted` cho telemetry |
| `firmware/fish_tank/net.cpp` | 1, 3 | Telemetry `null`, `muted`, nhận `cmd/mute`, event mới |
| `firmware/fish_tank/fish_tank.ino` | 1, 3, 4 | Mặt nạ cờ 16 bit, xử lý mute, so sánh config, điều kiện khởi động lại |
| `docs/app.js` | 1, 2, 3 | Chữ cờ mới, lịch mặc định, nút và event tắt tiếng |
| `docs/index.html` | 2, 3 | Giờ cho ăn mặc định, thanh tắt tiếng |
| `docs/style.css` | 3 | Kiểu `.mutebar` |
| `docs/sw.js` | 3 | Tăng phiên bản cache |
| `tools/fake-esp32/fake_esp32.py` | 1, 2, 3 | `fail turb`, `fail level`, lịch mặc định, `cmd/mute`, trường `muted` |
| `tools/test-risk/test_risk.cpp` | 1 | Ca thử khoảng đệm, biên nguy hiểm, cờ mới (35 lên 72 ca) |
| `tools/test-sensors-alarm/test_sensors_alarm.cpp` | 1, 3 | Ca thử `sensorsLevelOk`, lỗi cảm biến, còi mức 3 (83 lên 101 ca) |
| `tools/test-store-feeder/test_store_feeder.cpp` | 1, 2, 4 | Ca thử lịch mặc định, biên nguy hiểm, `configEqual` (76 lên 89 ca) |
| `tools/mqtt-samples/config-default.json` | 2 | Khớp mặc định firmware |
| `tools/mqtt-samples/README.md` | 3 | Dòng mẫu `cmd-*.json` |
| `tools/mqtt-samples/cmd-mute.json, cmd-unmute.json, event-mute.json, event-unmute.json, event-mute-rejected.json` | 3 | Tệp mẫu mới |
| `tools/mqtt-samples/telemetry-normal.json, telemetry-warning.json, telemetry-danger.json, telemetry-sensor-error.json` | 3 | Thêm `"muted": false` |
| `README.md` | 5 | Cập nhật số ca thử của giai đoạn 3 |

Tệp **không** được sửa (để tránh nhầm): `firmware/fish_tank/secrets.h`, `firmware/fish_tank/secrets.example.h`, `firmware/fish_tank/feeder.*`, `docs/manifest.json`, `docs/icons/*`, `tests/phase*/results.md`, mọi tệp `*.exe`.

---

## 3. Chi tiết các bước thực thi (Step-by-Step Implementation)

Thứ tự bắt buộc: Phase 0 (kế hoạch docx) → Phase 1 (Risk và cờ lỗi cảm biến) → Phase 2 (lịch cho ăn) → Phase 3 (còi và tắt tiếng) → Phase 4 (độ bền và NVS) → Phase 5 (kiểm thử và xác nhận). Mỗi tệp có bảng "Vị trí tác động, Thao tác" và khối diff chính xác.


## Phase 0: Cập nhật kế hoạch `IoT-fish-tank-plan.docx` lên bản 1.4

**Tệp:** `documentation/IoT-fish-tank-plan.docx`. **Mục tiêu:** để mục E (nguồn sự thật duy nhất) khớp với quyết định của nhóm trước khi sửa code (quy tắc A1). Sửa trực tiếp trong docx, giữ nguyên định dạng và bảng; sau khi sửa, mở lại bằng `pandoc documentation/IoT-fish-tank-plan.docx -t markdown` hoặc `python-docx` để kiểm tra không mất bảng, không có ký tự lạ, và không có mật khẩu. Nếu có skill docx thì đọc skill đó trước. Tìm đoạn cần sửa bằng **chuỗi neo** ở cột 2 (không dựa vào số trang).

Thao tác ở đây đều là **SỬA** hoặc **THÊM** đoạn văn bản; không có thao tác XÓA toàn mục.

| # | Mục | Neo hiện tại trong docx | Thao tác và nội dung mới |
|---|---|---|---|
| 0.1 | Đầu tài liệu | `Phiên bản 1.3 · Cập nhật ngày 31/10/2026` | **SỬA** thành `Phiên bản 1.4 · Cập nhật ngày 01/11/2026`. **THÊM** ngay sau đoạn bắt đầu bằng `Bản 1.3 (31/10/2026):` một đoạn mới: `Bản 1.4 (01/11/2026): nhóm đã chốt các quyết định sửa đối chiếu (khoảng đệm hạ mức 0,5 °C, 0,3 pH, 5 điểm %; khoảng đệm cho biên nguy hiểm; còi mức 3 là 3 tiếng mỗi 5 giây; lịch cho ăn mặc định 08:00, 14:00, 18:00; lỗi cảm biến độ đục và mực nước tính mức 1; lệnh tắt tiếng cmd/mute và nút trên web; khởi động lại thông minh; không ghi NVS khi cấu hình không đổi). Chi tiết ở documentation/fix-plan.md. Phần nào chạy trên máy tính hoặc ESP32 giả vẫn ghi là MÔ PHỎNG.` |
| 0.2 | E3. Topic và quyền | Bảng topic, dòng `fishtank/tank01/cmd/feed   webuser   esp32   QoS 1   Lệnh cho ăn` | **THÊM** dòng ngay sau: `fishtank/tank01/cmd/mute` / `webuser` / `esp32` / `QoS 1` / `Tắt tiếng hoặc bật lại tiếng còi (không retained)`. **SỬA** cột mô tả của dòng `fishtank/tank01/event`: thêm `, tắt tiếng` sau `cảnh báo`. Bảng quyền ACL (`cmd/#`) **không đổi**; thêm một câu: `cmd/mute nằm trong cmd/#, không cần sửa ACL trên EMQX.` |
| 0.3 | E4. JSON telemetry | Ví dụ JSON bắt đầu `{"v":1,"ts":1793236574,...` và bảng trường (`risk`, `flags`, `rssi`...) | **SỬA** ví dụ: thêm `"muted":false,` ngay sau `"risk":0,`. **THÊM** vào bảng trường một dòng sau `risk`: `muted` / `boolean` / `true khi còi đang tắt tiếng; trường tùy chọn, web coi thiếu là false`. **SỬA** mô tả `turb_v`, `turb_pct`, `level_pct`: thêm `null nếu cảm biến lỗi`. **SỬA** dòng `flags`: thêm `sensor_turb, sensor_level` vào cuối danh sách ví dụ (sau `sensor_ph`). |
| 0.4 | E4. Lệnh và event mới | Sau mục `### cmd/feed (web gửi, QoS 1)` và trong mục `### event (ESP32 gửi)` | **THÊM** mục mới `### cmd/mute (web gửi, QoS 1, KHÔNG retained)` với ví dụ `{"action":"mute"}` và `{"action":"unmute"}`, kèm câu: `Không bao giờ publish cmd/mute với retain, vì lệnh retained sẽ bị giao lại mỗi lần ESP32 nối lại. ESP32 bỏ qua payload rỗng, JSON lỗi và action lạ.` Trong `### event`, **THÊM** ba ví dụ: `{"type":"mute","minutes":30,"ts":1793236700}`, `{"type":"unmute","ts":1793236760}`, `{"type":"mute_rejected","reason":"no_alarm","ts":1793236800}`, và câu: `reason của mute_rejected: no_alarm (hiện không có cảnh báo), disabled (mute_min = 0). Event mute_rejected gửi ngay khi bị từ chối; telemetry có trường muted phản ánh trạng thái.` Sau đoạn `**Bản 1.3 (theo code):**` **THÊM** đoạn `**Bản 1.4:**` nêu: topic cmd/mute, trường muted, ba event mới, turb_v/turb_pct/level_pct có thể null. |
| 0.5 | E4. config mẫu | Ví dụ `"feed_times":["08:00","18:00"]` | **SỬA** thành `"feed_times":["08:00","14:00","18:00"]`. |
| 0.6 | E6. Giá trị mặc định | Hàng `Cho ăn` | **SỬA**: thêm vào cuối ô giá trị: `Lịch mặc định khi chưa có config: 3 mốc 08:00, 14:00, 18:00 (nhóm chốt bản 1.4); web và firmware cùng mặc định này.` |
| 0.7 | E6 | Hàng `Còi` | **SỬA** ô giá trị thành: `1300 Hz. Mức 1: 1 tiếng ngắn nhỏ 200 ms mỗi 60 giây. Mức 2: 2 tiếng nhỏ 200 ms cách nhau 200 ms, mỗi 30 giây. Mức 3: to nhất (duty khoảng 50%), 3 tiếng 200 ms cách nhau 200 ms liền nhau, lặp mỗi 5 giây. Tắt tiếng 30 phút (nút web hoặc cmd/mute): tự hủy khi risk tăng thêm hoặc về 0; risk giảm thì vẫn tắt tiếng.` Giữ nguyên ô ghi chú (duty chọn bằng tai ở T2.7, chưa kiểm chứng). |
| 0.8 | E6 | Dòng nói về `khởi động lại nếu mất MQTT quá 10 phút` (cũng có ở bảng Độ bền trong E11 và G, Giai đoạn 3, mục 11 "Ổn định") | **SỬA**: `Còn WiFi mà mất MQTT quá 10 phút thì khởi động lại; mất cả WiFi thì chờ 30 phút; không khởi động lại khi đang có cảnh báo (risk lớn hơn 0) hoặc servo đang quay.` |
| 0.9 | E8. Bảng mức | Hàng `Cảm biến lỗi (nhiệt độ, pH)` | **SỬA** nhãn thành `Cảm biến lỗi (nhiệt độ, pH, độ đục, mực nước)` và ô mức 1 thành `mức 1 (cờ sensor_temp, sensor_ph, sensor_turb, sensor_level)`. |
| 0.10 | E8. Quy tắc đi kèm | Đoạn `Cách hiểu "lệch"` (hiện bị cụt câu ở giữa, câu `Nếu nhóm muốn tính từ mép khoảng 25 đến 27 trở lên` không hoàn chỉnh) | **SỬA** cả đoạn thành: `Cách hiểu "lệch": tính so với giá trị chuẩn 26 °C và pH 7,0 (nhóm chốt giữ cách này, bản 1.4). Khoảng thích hợp 25 đến 27 °C chỉ dùng để vẽ vùng xanh trên web, không tham gia tính risk. Luật tổng hợp nhiều cờ dùng "từ n cờ mức 1 trở lên" (không phải "đúng mức n") để luật không bị đảo ngược: tình trạng nặng hơn không bao giờ báo nhẹ hơn. Cờ lỗi cảm biến cũng được tính.` (Phải đọc phần liền kề trong docx để giữ đúng câu về luật tổng hợp đang có; chỉ thay phần bị cụt.) |
| 0.11 | E8 | Đoạn `Cảm biến lỗi: nhiệt độ (nhóm chốt) và pH ...` | **SỬA** thành: `Cảm biến lỗi: nhiệt độ, pH, độ đục và mực nước đều tính mức 1 (cờ sensor_temp, sensor_ph, sensor_turb, sensor_level), nhóm chốt bản 1.4. Độ đục lỗi khi điện áp ngoài dải hợp lệ. Mực nước lỗi khi trung bình thô từ LEVEL_RAW_SHORT trở lên (chập, kẹt trần) hoặc chênh giữa mẫu lớn nhất và nhỏ nhất vượt LEVEL_SPREAD_MAX; hai hằng số này là giá trị TẠM, chốt khi hiệu chuẩn (T2.2, T2.3). Giá trị thô 0 vẫn là mực nước khô và báo level_low, không phải lỗi cảm biến.` Xóa nhãn **CẦN XÁC NHẬN** của đoạn này. |
| 0.12 | E8 | Đoạn `Còi: ba mức âm thanh theo mục E6 (...to nhất và liên tục)` | **SỬA**: `Còi: ba mức âm thanh theo mục E6 (nhỏ; 2 nhịp nhỏ; to nhất, 3 tiếng liền nhau mỗi 5 giây).` |
| 0.13 | E8. Bảng D1 đến D9 | Dòng D3 | **SỬA** nội dung: `Nâng risk khi điều kiện đúng 3 lần đánh giá liên tiếp (mỗi 5 giây, khoảng 10 đến 15 giây); hạ risk khi quay lại sâu trong ngưỡng một khoảng: 0,5 °C, 0,3 pH, 5 điểm %. Biên nguy hiểm cũng có khoảng đệm: nhiệt độ hạ từ mức 3 xuống mức 2 khi ≤ 32,5 hoặc ≥ 18,5 °C; pH hạ từ mức 2 xuống mức 1 khi ≤ 8,2 hoặc ≥ 5,3. Khoảng đệm tự giảm nếu ngưỡng cấu hình quá hẹp.` Trạng thái: `✔ Nhóm chốt 01/11/2026 (MÔ PHỎNG, chưa thử trên thiết bị)`. |
| 0.14 | E8 | Dòng D4 | **SỬA** nội dung: thêm `Độ đục và mực nước lỗi cũng mức 1 (xem đoạn Cảm biến lỗi).` Trạng thái: `✔ Nhóm chốt giữ nguyên`. |
| 0.15 | E8 | Dòng D5, D6, D7 | **SỬA** trạng thái: D5 `✔ Nhóm chốt giữ nguyên`; D6 và D7 `✔ Nhóm chốt 01/11/2026`. Ở D6 thêm: `Mặc định khi chưa có config: 08:00, 14:00, 18:00.` |
| 0.16 | E8 | Dòng D9 | **SỬA** nội dung: `Ba mức: nhỏ; 2 nhịp nhỏ; mức 3 to nhất 3 tiếng liền nhau mỗi 5 giây. Tắt tiếng 30 phút, hủy khi risk tăng hoặc về 0, giữ khi risk giảm; có nút web và lệnh cmd/mute.` Trạng thái: `✔ Nhóm chốt 01/11/2026 (code: MÔ PHỎNG; tiếng còi thật chưa thử)`. |
| 0.17 | E8 | Đoạn `**Bản 1.3:** các dòng ĐỀ XUẤT ở trên...` | **THÊM** một đoạn `**Bản 1.4:**` liền sau: `Nhóm đã trả lời H2 và các câu hỏi đối chiếu; các dòng D3 đến D9 đã chuyển sang trạng thái Nhóm chốt. Code đã cập nhật theo documentation/fix-plan.md.` |
| 0.18 | E10 | Bảng kiểm thử logic trên máy tính (hàng `test_risk.cpp`, `test_sensors_alarm.cpp`, `test_store_feeder.cpp`) | **SỬA** số ca: `35/35` thành `72/72`, `83 ca` thành `101 ca`, `76 ca` thành `89 ca`, kèm ghi chú `(bộ ca thử bản 1.4, chạy trên bản sao ngày 01/11/2026 bởi agent: MÔ PHỎNG; nhóm cần chạy lại và chụp ảnh mới, ảnh cũ ứng với bộ 35/83/76 ca)`. |
| 0.19 | E11 | Bảng `Đối chiếu code với kế hoạch (bản 1.3)` | Giữ tiêu đề và các dòng cũ. **SỬA** cột "Việc cần làm" của các dòng: `Lịch cho ăn` thành `Đã sửa bản 1.4: mặc định 08:00, 14:00, 18:00`; `Tắt tiếng còi` thành `Đã sửa bản 1.4: nút web và cmd/mute; T3.9 phần tắt tiếng thử được với ESP32 giả, thiết bị thật chưa thử`; `Lỗi cảm biến` thành `Đã sửa bản 1.4: thêm sensor_turb, sensor_level`; `Chống nhấp nháy (D3)` thành `Đã sửa bản 1.4: hạ khi quay lại sâu 0,5 °C, 0,3 pH, 5 điểm %; có khoảng đệm ở biên nguy hiểm`; `Subscribe của ESP32` thành `Đã sửa bản 1.4: thêm cmd/mute`; `Độ bền` thành `Đã sửa bản 1.4: khởi động lại 10 phút khi còn WiFi, 30 phút khi mất WiFi, không khi có cảnh báo; không ghi NVS khi config không đổi`. Với `Event config_rejected` thêm `web đã có chữ tiếng Việt (bản 1.4)`. |
| 0.20 | F5 | Bảng trạng thái code, hàng `Hàm tính risk và bảng ca thử E8 (T3.11)`, `Cảm biến và còi...`, `Cho ăn và lưu cấu hình...` | **SỬA** cột bằng chứng: thay `35/35 ca`, `83 ca`, `76 ca` bằng `72/72 ca (bộ bản 1.4)`, `101 ca`, `89 ca`, cộng câu `Số ca cũ 35/83/76 có ảnh ở tests/phase3/; bộ mới chưa có ảnh (nhóm chụp lại)`. **KHÔNG** sửa khối `Cảnh báo bảo mật` và dòng sau nó. |
| 0.21 | G. Giai đoạn 3 | Hàng `T3.9` (cột "Kết quả đúng") | **SỬA**: `Còi kêu đúng mẫu cảnh báo, rồi nguy hiểm; tắt tiếng 30 phút hoạt động. Bản 1.4: mức 3 là 3 tiếng to nhất liền nhau mỗi 5 giây; nút Tắt tiếng trên web và lệnh cmd/mute làm còi im; tắt tiếng hủy khi risk tăng.` |
| 0.22 | G. Giai đoạn 3 | Sau hàng `T3.11` và dòng `Hoàn thành khi: T3.1 đến T3.11 đạt` | **THÊM** hàng `T3.12`: việc `Khi đang có cảnh báo, bấm Tắt tiếng trên web (hoặc publish cmd/mute)`; kết quả đúng `Còi im, event mute, telemetry muted = true; risk tăng thì còi kêu lại, muted = false`; ghi chú `Thử trước với ESP32 giả (MÔ PHỎNG), rồi thiết bị thật`. **SỬA** `T3.1 đến T3.11` thành `T3.1 đến T3.12` (cả ở dòng Hoàn thành khi lẫn nơi khác nếu có). |
| 0.23 | G. Danh sách kiểm cuối | Dòng bắt đầu `☐ Còi ba mức (nhỏ; 2 nhịp nhỏ; to nhất liên tục)` | **SỬA**: `☐ Còi ba mức (nhỏ; 2 nhịp nhỏ; to nhất, 3 tiếng liền nhau mỗi 5 giây) đúng mẫu; tắt tiếng hoạt động; bảng ca thử risk (mục E8) đạt trên firmware.` |
| 0.24 | H2 | Mục `H2. Câu hỏi cần nhóm xác nhận` (phần "Nhóm đã trả lời...") | **Đọc kỹ H2 trước.** Với mỗi câu liên quan lịch cho ăn mặc định, lỗi cảm biến độ đục và mực nước, cách kích hoạt tắt tiếng: **SỬA** nhãn **CẦN XÁC NHẬN** thành `✔ Nhóm chốt` kèm giá trị ở mục 1.2 của `fix-plan.md`. **THÊM** ngay cuối H2 mục `Quyết định bổ sung 01/11/2026` ghi lại: khoảng đệm hạ mức 0,5 / 0,3 / 5; hướng A cho biên nguy hiểm; còi mức 3; tắt tiếng 30 phút; và sáu giả định ở mục 1.5 của `fix-plan.md` với nhãn **CẦN XÁC NHẬN** (để nhóm duyệt). Không đưa câu hỏi mới nào ngoài các giả định đó. |
| 0.25 | H5. Nhật ký thay đổi | Cuối bảng `Nhật ký thay đổi so với các bản kế hoạch trước` | **THÊM** các dòng (Nội dung / Trước / Sau / Lý do): `Khoảng đệm hạ mức` / `0,2 °C, 0,1 pH, 2 điểm %` / `0,5 °C, 0,3 pH, 5 điểm %` / `Nhóm chốt, chống nhấp nháy mạnh hơn`; `Còi mức 3` / `Kêu to nhất liên tục` / `3 tiếng to nhất mỗi 5 giây` / `Nhóm chốt`; `Lịch cho ăn mặc định` / `Không có mốc nào` / `08:00, 14:00, 18:00` / `Nhóm chốt`; `Lỗi cảm biến độ đục, mực nước` / `Chưa quy định` / `Mức 1` / `Nhóm chốt`; `Tắt tiếng còi` / `Có logic, chưa có đường kích hoạt` / `Nút web và cmd/mute` / `Nhóm chốt`; `Khởi động lại khi mất MQTT` / `10 phút` / `10 phút (còn WiFi), 30 phút (mất WiFi), không khi có cảnh báo` / `Tránh ngắt còi`; `Ghi NVS khi nhận config` / `Mỗi lần nhận` / `Chỉ khi khác` / `Giảm mòn flash`. |
| 0.26 | Toàn tài liệu | Chuỗi `1.3` ở những chỗ chỉ phiên bản tài liệu (không phải "bản 1.3" của các đoạn lịch sử), `to nhất liên tục`, `08:00, 18:00`, `0,2 °C, 0,1 pH, 2 điểm` | **Tìm bằng pandoc/grep trên bản markdown tạm** và **SỬA** các chỗ còn sót theo cùng nội dung ở trên; không sửa các đoạn mô tả lịch sử có nhãn `bản 1.2` hay `bản 1.3`. |

**Kiểm tra sau Phase 0:** mở lại docx, đối chiếu bảng trên, bảo đảm mục E3, E4, E6, E8, E10, E11, F5, G, H2, H5 đã sửa, các bảng không vỡ, `grep` trong bản markdown tạm không còn `to nhất liên tục`, `08:00, 18:00` ngoài đoạn lịch sử. Không có mật khẩu trong tài liệu.

**Commit gợi ý:** `docs: cap nhat IoT-fish-tank-plan len ban 1.4 theo quyet dinh cua nhom` (thêm cả `documentation/fix-plan.md`).


---

## Phase 1: Risk, khoảng đệm hạ mức và cờ lỗi cảm biến (D3, #9, #2, #3)

**Điều kiện:** Phase 0 đã xong. Kho sạch (`git status` không có thay đổi lạ ngoài `docs/icons/icon.svg`).

### Bước 1.1 Hằng số, cờ và cấu trúc của risk (lõi, thuần C++)

Đổi khoảng đệm hạ mức (0,5 °C, 0,3 pH, 5 điểm %), thêm hai cờ lỗi cảm biến (`FL_COUNT` từ 8 lên 10), đưa bốn biên nguy hiểm vào `RiskThresholds` để `riskTighten()` thu hẹp được (hướng A của #9), và thêm hai biên bảo vệ để ngưỡng thu hẹp không bao giờ chồng nhau khi ngưỡng cấu hình hẹp bất thường.

#### Tệp: `firmware/fish_tank/config.h`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | khối `HYST_*` (chống nhấp nháy D3), đổi giá trị 3 hằng số | **SỬA** | `-79,9 +79,9` |
| 2 | ngay sau `LEVEL_PWR_SETTLE_MS`: thêm hằng số ngưỡng lỗi cảm biến mực nước | **THÊM** | `-115,6 +115,8` |

```diff
--- a/firmware/fish_tank/config.h
+++ b/firmware/fish_tank/config.h
@@ -79,9 +79,9 @@
 // Chống nhấp nháy (D3, ĐỀ XUẤT phương án A)
 // ---------------------------------------------------------------------------
 #define RISK_RAISE_READS        3        // nâng mức khi điều kiện đúng 3 lần đánh giá liên tiếp
-#define HYST_TEMP               0.2f     // hạ mức khi quay lại trong ngưỡng một khoảng nhỏ
-#define HYST_PH                 0.1f
-#define HYST_PCT                2.0f
+#define HYST_TEMP               0.5f     // hạ mức khi quay lại sâu trong ngưỡng: 0,5 °C (nhóm chốt, xem documentation/fix-plan.md)
+#define HYST_PH                 0.3f     // 0,3 pH
+#define HYST_PCT                5.0f     // 5 điểm % (độ đục, mực nước)
 
 // ---------------------------------------------------------------------------
 // Đọc cảm biến
@@ -115,6 +115,8 @@
 #define LEVEL_RAW_DRY           0          // giá trị ADC khi khô
 #define LEVEL_RAW_FULL          2500       // giá trị ADC khi ngập tới mức đầy
 #define LEVEL_PWR_SETTLE_MS     20         // chờ sau khi bật GPIO27 rồi mới đọc
+#define LEVEL_RAW_SHORT         4090       // GIÁ TRỊ TẠM: trung bình thô từ mức này trở lên = chập hoặc kẹt trần ADC -> lỗi cảm biến mực nước (phải lớn hơn LEVEL_RAW_FULL)
+#define LEVEL_SPREAD_MAX        300        // GIÁ TRỊ TẠM: chênh giữa mẫu lớn nhất và nhỏ nhất trong một vòng đo vượt mức này = đọc không ổn định -> lỗi
 
 // ---------------------------------------------------------------------------
 // Servo cho ăn (E6)
```

#### Tệp: `firmware/fish_tank/risk.h`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | `enum` cờ: thêm hai cờ sau `FL_SENSOR_PH` | **THÊM** | `-19,6 +19,8` |
| 2 | `struct RiskThresholds`: thêm 4 biên nguy hiểm | **THÊM** | `-28,6 +30,8` |
| 3 | `struct RiskInput`: thêm `turb_ok`, `level_ok` | **SỬA** | `-35,7 +39,9` |

```diff
--- a/firmware/fish_tank/risk.h
+++ b/firmware/fish_tank/risk.h
@@ -19,6 +19,8 @@
   FL_LEVEL_LOW,
   FL_SENSOR_TEMP,
   FL_SENSOR_PH,
+  FL_SENSOR_TURB,        // lỗi cảm biến độ đục (mức 1)
+  FL_SENSOR_LEVEL,       // lỗi cảm biến mực nước (mức 1)
   FL_COUNT
 };
 
@@ -28,6 +30,8 @@
   float turb_max_pct;
   float level_min_pct;
   float level_danger_pct;
+  float temp_danger_min, temp_danger_max;   // biên nguy hiểm (mức 3 nhiệt độ); riskTighten() thu hẹp để hạ mức có khoảng đệm
+  float ph_danger_min, ph_danger_max;       // biên nguy hiểm (mức 2 pH)
 };
 
 struct RiskInput {           // số liệu đã lọc và hiệu chuẩn
@@ -35,7 +39,9 @@
   float temp_c;
   bool  ph_ok;               // false nếu cảm biến pH lỗi (ADC ngoài dải)
   float ph;
-  float turb_pct;            // lỗi cảm biến độ đục và mực nước: chưa quy định (CẦN XÁC NHẬN)
+  bool  turb_ok;             // false nếu cảm biến độ đục lỗi (điện áp ngoài dải)
+  float turb_pct;
+  bool  level_ok;            // false nếu cảm biến mực nước lỗi (chập, kẹt trần, đọc không ổn định)
   float level_pct;
 };
 
```

#### Tệp: `firmware/fish_tank/risk.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | mảng `FLAG_NAMES` (đầu tệp) | **SỬA** | `-6,7 +6,8` |
| 2 | `riskDefaultThresholds()` (thêm 4 biên nguy hiểm) và thay toàn bộ hàm `riskTighten()` | **SỬA** | `-22,19 +23,38` |
| 3 | `riskComputeLevels()`, khối nhiệt độ: dòng so với `TEMP_DANGER_MIN/MAX` | **SỬA** | `-48,7 +68,7` |
| 4 | `riskComputeLevels()`: dòng pH so với `PH_DANGER_MIN/MAX`, rồi khối độ đục và khối mực nước | **SỬA** | `-60,18 +80,28` |

```diff
--- a/firmware/fish_tank/risk.cpp
+++ b/firmware/fish_tank/risk.cpp
@@ -6,7 +6,8 @@
 
 static const char* const FLAG_NAMES[FL_COUNT] = {
   "temp_high", "temp_low", "ph_high", "ph_low",
-  "turb_high", "level_low", "sensor_temp", "sensor_ph"
+  "turb_high", "level_low", "sensor_temp", "sensor_ph",
+  "sensor_turb", "sensor_level"
 };
 
 const char* riskFlagName(uint8_t flag) {
@@ -22,19 +23,38 @@
   t.turb_max_pct = DEF_TURB_MAX_PCT;
   t.level_min_pct = DEF_LEVEL_MIN_PCT;
   t.level_danger_pct = DEF_LEVEL_DANGER_PCT;
+  t.temp_danger_min = TEMP_DANGER_MIN;
+  t.temp_danger_max = TEMP_DANGER_MAX;
+  t.ph_danger_min = PH_DANGER_MIN;
+  t.ph_danger_max = PH_DANGER_MAX;
   return t;
 }
 
+// Khoảng đệm không được vượt một phần tư độ rộng vùng an toàn, để ngưỡng thu hẹp không bao giờ chồng lên nhau.
+static float hystMargin(float want, float span) {
+  float cap = span * 0.25f;
+  return want < cap ? want : cap;
+}
+
 // Ngưỡng "chặt hơn" một khoảng nhỏ: giá trị phải nằm sâu trong vùng an toàn thì mới được hạ mức.
+// Khoảng đệm theo quyết định của nhóm (documentation/fix-plan.md): 0,5 °C, 0,3 pH, 5 điểm %; áp dụng cả cho biên nguy hiểm (#9, hướng A).
 RiskThresholds riskTighten(const RiskThresholds& th) {
   RiskThresholds t = th;
-  t.temp_min += HYST_TEMP;
-  t.temp_max -= HYST_TEMP;
-  t.ph_min += HYST_PH;
-  t.ph_max -= HYST_PH;
-  t.turb_max_pct -= HYST_PCT;
+  float ht = hystMargin(HYST_TEMP, th.temp_max - th.temp_min);
+  float hp = hystMargin(HYST_PH, th.ph_max - th.ph_min);
+  t.temp_min += ht;
+  t.temp_max -= ht;
+  t.ph_min += hp;
+  t.ph_max -= hp;
+  t.temp_danger_min += HYST_TEMP;
+  t.temp_danger_max -= HYST_TEMP;
+  t.ph_danger_min += HYST_PH;
+  t.ph_danger_max -= HYST_PH;
+  t.turb_max_pct -= hystMargin(HYST_PCT, th.turb_max_pct * 2.0f);   // ngưỡng nhỏ thì đệm nhỏ lại (tối đa một nửa ngưỡng)
   t.level_min_pct += HYST_PCT;
+  if (t.level_min_pct > 100.0f) t.level_min_pct = 100.0f;
   t.level_danger_pct += HYST_PCT;
+  if (t.level_danger_pct > t.level_min_pct) t.level_danger_pct = t.level_min_pct;
   return t;
 }
 
@@ -48,7 +68,7 @@
   } else {
     float t = in.temp_c;
     uint8_t lv = 0;
-    if (t < TEMP_DANGER_MIN || t > TEMP_DANGER_MAX) lv = 3;
+    if (t < th.temp_danger_min || t > th.temp_danger_max) lv = 3;
     else if (t < th.temp_min - TEMP_L2_EXTRA || t > th.temp_max + TEMP_L2_EXTRA) lv = 2;
     else if (t < th.temp_min || t > th.temp_max) lv = 1;
     if (lv) out.lv[t < th.temp_min ? FL_TEMP_LOW : FL_TEMP_HIGH] = lv;
@@ -60,18 +80,28 @@
   } else {
     float p = in.ph;
     uint8_t lv = 0;
-    if (p < PH_DANGER_MIN || p > PH_DANGER_MAX) lv = 2;
+    if (p < th.ph_danger_min || p > th.ph_danger_max) lv = 2;
     else if (p < th.ph_min || p > th.ph_max) lv = 1;
     if (lv) out.lv[p < th.ph_min ? FL_PH_LOW : FL_PH_HIGH] = lv;
   }
 
-  // Độ đục: mức 1 trên ngưỡng; mức 2 trên ngưỡng + 15 điểm %
-  if (in.turb_pct > th.turb_max_pct + TURB_L2_EXTRA) out.lv[FL_TURB_HIGH] = 2;
-  else if (in.turb_pct > th.turb_max_pct) out.lv[FL_TURB_HIGH] = 1;
-
-  // Mực nước: mức 2 dưới level_danger; mức 1 dưới level_min
-  if (in.level_pct < th.level_danger_pct) out.lv[FL_LEVEL_LOW] = 2;
-  else if (in.level_pct < th.level_min_pct) out.lv[FL_LEVEL_LOW] = 1;
+  // Độ đục: mức 1 trên ngưỡng; mức 2 trên ngưỡng + 15 điểm %; cảm biến lỗi = mức 1 (nhóm chốt, xem documentation/fix-plan.md)
+  if (!in.turb_ok) {
+    out.lv[FL_SENSOR_TURB] = SENSOR_FAULT_LEVEL;
+  } else if (in.turb_pct > th.turb_max_pct + TURB_L2_EXTRA) {
+    out.lv[FL_TURB_HIGH] = 2;
+  } else if (in.turb_pct > th.turb_max_pct) {
+    out.lv[FL_TURB_HIGH] = 1;
+  }
+
+  // Mực nước: mức 2 dưới level_danger; mức 1 dưới level_min; cảm biến lỗi = mức 1
+  if (!in.level_ok) {
+    out.lv[FL_SENSOR_LEVEL] = SENSOR_FAULT_LEVEL;
+  } else if (in.level_pct < th.level_danger_pct) {
+    out.lv[FL_LEVEL_LOW] = 2;
+  } else if (in.level_pct < th.level_min_pct) {
+    out.lv[FL_LEVEL_LOW] = 1;
+  }
 }
 
 // Luật tổng hợp: lấy mức riêng cao nhất; từ 3 cờ (mức >= 1) thì ít nhất mức 2;
```

### Bước 1.2 Phát hiện lỗi cảm biến độ đục và mực nước

`turb_ok` = điện áp trong dải hợp lệ (dùng lại `sensorsAdcVoltOk`). `level_ok` = false khi trung bình thô ≥ `LEVEL_RAW_SHORT` (chập, kẹt trần) hoặc chênh giữa mẫu lớn nhất và nhỏ nhất > `LEVEL_SPREAD_MAX`. Giá trị thô 0 (khô hoặc hở mạch) vẫn là hợp lệ và báo `level_low`, vì báo thừa còn hơn im lặng khi cạn nước.

#### Tệp: `firmware/fish_tank/sensors.h`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | `struct SensorValues`: thêm `turb_ok`, `level_ok` | **SỬA** | `-25,8 +25,8` |
| 2 | khai báo hàm `sensorsLevelOk()` | **THÊM** | `-35,6 +35,7` |

```diff
--- a/firmware/fish_tank/sensors.h
+++ b/firmware/fish_tank/sensors.h
@@ -25,8 +25,8 @@
 struct SensorValues {
   bool  temp_ok;   float temp_c;                 // temp_c = NAN khi lỗi
   bool  ph_ok;     float ph;   float ph_v;       // ph = NAN khi lỗi; ph_v là điện áp module (V)
-  float turb_v;    float turb_pct;               // điện áp module (V) và độ đục tương đối 0 đến 100 %
-  int   level_raw; float level_pct;              // giá trị ADC thô và mực nước 0 đến 100 %
+  bool  turb_ok;   float turb_v;    float turb_pct;  // turb_ok = false nếu điện áp ngoài dải; turb_v (V), độ đục tương đối 0 đến 100 %
+  bool  level_ok;  int   level_raw; float level_pct; // level_ok = false nếu chập, kẹt trần hoặc đọc không ổn định; giá trị ADC thô và mực nước 0 đến 100 %
   bool  temp_ready;                              // đã có ít nhất một kết quả nhiệt độ
   bool  slow_ready;                              // đã có ít nhất một vòng pH, độ đục, mực nước
 };
@@ -35,6 +35,7 @@
 float sensorsTrimmedMean(const float* v, uint8_t n);       // bỏ mẫu nhỏ nhất và lớn nhất rồi lấy trung bình
 bool  sensorsTempValid(float c);                           // false nếu -127 (mất cảm biến), 85,0 (mặc định khi bật nguồn), NaN, ngoài dải
 bool  sensorsAdcVoltOk(float v);                           // điện áp module trong dải hợp lệ (dây đứt hoặc chập thì sai)
+bool  sensorsLevelOk(const float* raw, uint8_t n);         // false nếu không có mẫu, trung bình >= LEVEL_RAW_SHORT, hoặc (lớn nhất - nhỏ nhất) > LEVEL_SPREAD_MAX
 float sensorsPhFromVolt(float v, const SensorCal& cal);    // pH = 6,86 + (V - V686) x (6,86 - 4,01) / (V686 - V401); NAN nếu hiệu chuẩn hỏng
 float sensorsTurbPct(float v, const SensorCal& cal);       // 0 đến 100
 float sensorsLevelPct(float raw, const SensorCal& cal);    // 0 đến 100
```

#### Tệp: `firmware/fish_tank/sensors.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | thêm hàm `sensorsLevelOk()` ngay sau `sensorsAdcVoltOk()` | **THÊM** | `-40,6 +40,19` |
| 2 | `sensorsToRiskInput()` | **SỬA** | `-69,8 +82,10` |
| 3 | `SensorHub::finishChannel()`: nhánh `CH_TURB` và `CH_LEVEL` | **THÊM** | `-172,10 +187,12` |

```diff
--- a/firmware/fish_tank/sensors.cpp
+++ b/firmware/fish_tank/sensors.cpp
@@ -40,6 +40,19 @@
   return v >= ADC_V_MIN_VALID && v <= ADC_V_MAX_VALID;       // NaN cũng cho kết quả false
 }
 
+bool sensorsLevelOk(const float* raw, uint8_t n) {
+  if (n == 0) return false;
+  float sum = 0.0f, lo = raw[0], hi = raw[0];
+  for (uint8_t i = 0; i < n; i++) {
+    if (!(raw[i] == raw[i])) return false;                   // NaN
+    sum += raw[i];
+    if (raw[i] < lo) lo = raw[i];
+    if (raw[i] > hi) hi = raw[i];
+  }
+  if (sum / n >= (float)LEVEL_RAW_SHORT) return false;       // chập hoặc kẹt trần ADC
+  return (hi - lo) <= (float)LEVEL_SPREAD_MAX;               // đọc nhảy quá mạnh trong một vòng đo = không tin được
+}
+
 float sensorsPhFromVolt(float v, const SensorCal& cal) {
   float dv = cal.ph_v686 - cal.ph_v401;
   if (fabsf(dv) < 0.01f) return NAN;                         // hai điểm hiệu chuẩn trùng nhau: không tính được
@@ -69,8 +82,10 @@
   r.temp_c = s.temp_ok ? s.temp_c : 0.0f;
   r.ph_ok = s.ph_ok;
   r.ph = s.ph_ok ? s.ph : 0.0f;
-  r.turb_pct = s.turb_pct;
-  r.level_pct = s.level_pct;
+  r.turb_ok = s.turb_ok;
+  r.turb_pct = s.turb_ok ? s.turb_pct : 0.0f;
+  r.level_ok = s.level_ok;
+  r.level_pct = s.level_ok ? s.level_pct : 100.0f;
   return r;
 }
 
@@ -172,10 +187,12 @@
   } else if (chan_ == CH_TURB) {
     float v = mean / 1000.0f * ADC_DIVIDER;
     v_.turb_v = v;
+    v_.turb_ok = sensorsAdcVoltOk(v);
     v_.turb_pct = sensorsTurbPct(v, cal_);
     startChannel(CH_LEVEL, now);
   } else {
     hal_.levelPower(false);
+    v_.level_ok = sensorsLevelOk(buf_, nSamp_);
     v_.level_raw = (int)(mean + 0.5f);
     v_.level_pct = sensorsLevelPct(mean, cal_);
     v_.slow_ready = true;
```

### Bước 1.3 Lan truyền sang store, mạng và vòng lặp chính

`configThresholds()` phải gán 4 biên mới (nếu thiếu, giá trị không khởi tạo sẽ làm mọi cờ sai). Telemetry gửi `null` cho `turb_v`, `turb_pct`, `level_pct` khi cảm biến lỗi. Mặt nạ cờ phải là 16 bit vì có 10 cờ (nếu giữ 8 bit, hai cờ mới bị cắt cụt âm thầm và telemetry không gửi ngay khi chúng đổi).

#### Tệp: `firmware/fish_tank/store.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | `configThresholds()`: gán 4 biên nguy hiểm | **THÊM** | `-29,6 +29,10` |

```diff
--- a/firmware/fish_tank/store.cpp
+++ b/firmware/fish_tank/store.cpp
@@ -29,6 +29,10 @@
   t.turb_max_pct = c.turb_max_pct;
   t.level_min_pct = c.level_min_pct;
   t.level_danger_pct = c.level_danger_pct;
+  t.temp_danger_min = TEMP_DANGER_MIN;
+  t.temp_danger_max = TEMP_DANGER_MAX;
+  t.ph_danger_min = PH_DANGER_MIN;
+  t.ph_danger_max = PH_DANGER_MAX;
   return t;
 }
 
```

#### Tệp: `firmware/fish_tank/net.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | `netPublishTelemetry()`: ba lệnh `fmtNum` của `turbV`, `turb`, `level` (≈ dòng 167 đến 169) | **SỬA** | `-164,9 +164,9` |

```diff
--- a/firmware/fish_tank/net.cpp
+++ b/firmware/fish_tank/net.cpp
@@ -164,9 +164,9 @@
   char temp[16], ph[16], turbV[16], turb[16], level[16], flags[96], tsPart[24];
   fmtNum(temp, sizeof(temp), s.temp_ok, s.temp_c, 1);
   fmtNum(ph, sizeof(ph), s.ph_ok, s.ph, 2);
-  fmtNum(turbV, sizeof(turbV), true, s.turb_v, 2);
-  fmtNum(turb, sizeof(turb), true, s.turb_pct, 0);       // số nguyên như fake_esp32.py
-  fmtNum(level, sizeof(level), true, s.level_pct, 0);
+  fmtNum(turbV, sizeof(turbV), s.turb_ok, s.turb_v, 2);   // cảm biến lỗi thì null
+  fmtNum(turb, sizeof(turb), s.turb_ok, s.turb_pct, 0);   // số nguyên như fake_esp32.py
+  fmtNum(level, sizeof(level), s.level_ok, s.level_pct, 0);
   flagsJson(r.levels, flags, sizeof(flags));
   uint32_t ts = netEpoch();
   if (ts) snprintf(tsPart, sizeof(tsPart), "\"ts\":%lu,", (unsigned long)ts);
```

#### Tệp: `firmware/fish_tank/fish_tank.ino`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | khai báo biến toàn cục `g_prevFlags` (≈ dòng 32) | **SỬA** | `-29,7 +29,7` |
| 2 | vòng `loop()`, bước 2 (tính risk): biến `flagMask` (≈ dòng 179 đến 180) | **SỬA** | `-176,8 +176,8` |

```diff
--- a/firmware/fish_tank/fish_tank.ino
+++ b/firmware/fish_tank/fish_tank.ino
@@ -29,7 +29,7 @@
 static uint32_t g_nextSchedMs = 0;
 static uint32_t g_nextLogMs = 0;
 static uint8_t  g_prevRisk = 0;
-static uint8_t  g_prevFlags = 0;          // mặt nạ bit các cờ đang có (để gửi telemetry ngay khi cờ đổi)
+static uint16_t g_prevFlags = 0;         // mặt nạ bit các cờ đang có (để gửi telemetry ngay khi cờ đổi); 16 bit vì FL_COUNT = 10
 static uint32_t g_lastSchedRejectEpoch = 0;
 
 static bool due(uint32_t now, uint32_t target) {
@@ -176,8 +176,8 @@
   if (g_sensors.ready() && due(now, g_nextRiskMs)) {
     g_nextRiskMs = now + RISK_PERIOD_MS;
     g_risk = g_filter.update(sensorsToRiskInput(g_sensors.values()), configThresholds(g_cfg));
-    uint8_t flagMask = 0;
-    for (uint8_t i = 0; i < FL_COUNT; i++) if (g_risk.levels.lv[i]) flagMask |= (uint8_t)(1u << i);
+    uint16_t flagMask = 0;
+    for (uint8_t i = 0; i < FL_COUNT; i++) if (g_risk.levels.lv[i]) flagMask |= (uint16_t)(1u << i);
     if (g_risk.risk != g_prevRisk || flagMask != g_prevFlags) {
       Serial.printf("[risk] %u -> %u\n", (unsigned)g_prevRisk, (unsigned)g_risk.risk);
       if (g_risk.risk > g_prevRisk) netPublishAlarm(g_risk);   // event "alarm" khi risk TĂNG (như fake_esp32.py)
```

### Bước 1.4 Web và ESP32 giả

Web hiện chữ cho hai cờ mới và đánh dấu thẻ "Lỗi cảm biến" khi cờ bật. ESP32 giả mô phỏng `fail turb` và `fail level`.

#### Tệp: `docs/app.js`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | hằng `FLAG_TEXT`, sau dòng `sensor_temp: ..., sensor_ph: ...` | **THÊM** | `-41,6 +41,7` |
| 2 | mảng `METRICS`, hai mục `turb` và `level` (thuộc tính `sensor`) | **SỬA** | `-272,12 +273,12` |

```diff
--- a/docs/app.js
+++ b/docs/app.js
@@ -41,6 +41,7 @@
     ph_low: "pH thấp", ph_high: "pH cao",
     turb_high: "nước đục", level_low: "mực nước thấp",
     sensor_temp: "lỗi cảm biến nhiệt độ", sensor_ph: "lỗi cảm biến pH",
+    sensor_turb: "lỗi cảm biến độ đục", sensor_level: "lỗi cảm biến mực nước",
 };
 const REASON_TEXT = {
     too_soon: "quá sớm (phải chờ ít nhất 60 giây giữa hai lần)",
@@ -272,12 +273,12 @@
         word: (v, c, f) => (f && f.includes("ph_low")) ? "pH thấp (nước chua)" : (f && f.includes("ph_high")) ? "pH cao (nước kiềm)" : (v < c.ph_min ? "pH thấp (nước chua)" : "pH cao (nước kiềm)"),
     },
     {
-        key: "turb", get: d => d.turb_pct, digits: 0, unit: " %", flags: ["turb_high"], sensor: null,
+        key: "turb", get: d => d.turb_pct, digits: 0, unit: " %", flags: ["turb_high"], sensor: "sensor_turb",
         sev: sevTurb, tone: toneTurb,
         word: () => "Nước đục",
     },
     {
-        key: "level", get: d => d.level_pct, digits: 0, unit: " %", flags: ["level_low"], sensor: null,
+        key: "level", get: d => d.level_pct, digits: 0, unit: " %", flags: ["level_low"], sensor: "sensor_level",
         sev: sevLevel, tone: toneLevel,
         word: (v, c, f, lv) => lv >= 2 ? "Mực nước rất thấp" : "Mực nước thấp",
     },
```

#### Tệp: `tools/fake-esp32/fake_esp32.py`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | hằng `SENSOR_FAULT_LEVEL` (chú thích) | **SỬA** | `-75,7 +75,7` |
| 2 | hàm `evaluate()`: khối độ đục và mực nước | **SỬA** | `-157,13 +157,17` |
| 3 | chuỗi `HELP`, dòng lệnh `fail` | **SỬA** | `-236,7 +240,7` |
| 4 | phương thức `FakeEsp32.sample()` | **SỬA** | `-334,10 +338,12` |
| 5 | `handle_command()`: nhánh `fail` | **SỬA** | `-439,7 +445,7` |

```diff
--- a/tools/fake-esp32/fake_esp32.py
+++ b/tools/fake-esp32/fake_esp32.py
@@ -75,7 +75,7 @@
 # Mỗi cờ pH vẫn được tính vào luật tổng hợp nhiều cờ (hàm aggregate).
 TEMP_DANGER = (18.0, 33.0)
 PH_DANGER = (5.0, 8.5)
-SENSOR_FAULT_LEVEL = 1  # cảm biến lỗi (temp_c / ph = null) -> mức 1
+SENSOR_FAULT_LEVEL = 1  # cảm biến lỗi (temp_c / ph / turb_pct / level_pct = null) -> mức 1
 
 # Luật cho ăn
 MAX_PORTIONS = 2        # số phần tối đa mỗi lệnh (kẹp xuống nếu lớn hơn)
@@ -157,13 +157,17 @@
             levels["ph_low" if p < lo else "ph_high"] = lv
 
     tb = r["turb_pct"]
-    if tb > cfg["turb_max_pct"] + TURB_L2_EXTRA:
+    if tb is None:
+        levels["sensor_turb"] = SENSOR_FAULT_LEVEL
+    elif tb > cfg["turb_max_pct"] + TURB_L2_EXTRA:
         levels["turb_high"] = 2
     elif tb > cfg["turb_max_pct"]:
         levels["turb_high"] = 1
 
     lvl = r["level_pct"]
-    if lvl < cfg["level_danger_pct"]:
+    if lvl is None:
+        levels["sensor_level"] = SENSOR_FAULT_LEVEL
+    elif lvl < cfg["level_danger_pct"]:
         levels["level_low"] = 2
     elif lvl < cfg["level_min_pct"]:
         levels["level_low"] = 1
@@ -236,7 +240,7 @@
     demo3b               2 cờ mức 2                     -> risk 3
   set <temp|ph|turb|level> <số>   đặt giá trị tuỳ ý
                          ví dụ: set temp 32 (mức 2), set temp 35 (mức 3), set ph 8.7 (mức 2)
-  fail <temp|ph>         giả lỗi cảm biến (giá trị null + cờ sensor_*, mức 1)
+  fail <temp|ph|turb|level>  giả lỗi cảm biến (giá trị null + cờ sensor_*, mức 1)
   fix                    sửa mọi lỗi cảm biến
   boot                   gửi lại sự kiện boot
   crash                  thoát đột ngột (không DISCONNECT) -> broker gửi Last Will "offline"
@@ -334,10 +338,12 @@
         ph = None if "ph" in self.fail else round(b["ph"] + random.gauss(0, 0.02), 2)
         turb = clamp(b["turb"] + random.gauss(0, 0.4), 0, 100)
         level = clamp(b["level"] + random.gauss(0, 0.4), 0, 100)
+        turb_bad, level_bad = "turb" in self.fail, "level" in self.fail
         return {
             "temp_c": temp, "ph": ph,
-            "turb_v": round(V_CLEAR - (V_CLEAR - V_DIRTY) * turb / 100.0, 2),
-            "turb_pct": int(round(turb)), "level_pct": int(round(level)),
+            "turb_v": None if turb_bad else round(V_CLEAR - (V_CLEAR - V_DIRTY) * turb / 100.0, 2),
+            "turb_pct": None if turb_bad else int(round(turb)),
+            "level_pct": None if level_bad else int(round(level)),
         }
 
     def publish_telemetry(self):
@@ -439,7 +445,7 @@
                 log(f"OK: {p[1]} = {p[2]}")
             except ValueError:
                 log("Giá trị phải là số. Ví dụ: set temp 35")
-        elif c == "fail" and len(p) == 2 and p[1] in ("temp", "ph"):
+        elif c == "fail" and len(p) == 2 and p[1] in ("temp", "ph", "turb", "level"):
             self.fail.add(p[1])
             self.force_publish = True
             log(f"OK: giả lỗi cảm biến {p[1]}")
```

### Bước 1.5 Bộ ca thử

Cập nhật `normalInput()` (nếu không, `turb_ok`/`level_ok` không khởi tạo sẽ làm mọi ca báo lỗi), đổi hai ca hạ mức theo khoảng đệm mới, thêm ca cho cờ mới, khoảng đệm ở biên nguy hiểm và bảo vệ ngưỡng hẹp. Đây là thay đổi bảng ca thử có chủ đích theo quyết định của nhóm, không phải sửa bảng cho khớp code.

#### Tệp: `tools/test-risk/test_risk.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | hàm `normalInput()` | **SỬA** | `-15,8 +15,8` |
| 2 | thêm hàm `expectTrue()` ngay trước `main()` | **THÊM** | `-31,6 +31,12` |
| 3 | bảng ca thử E8: thêm ca lỗi cảm biến sau ca `Cảm biến pH lỗi` | **THÊM** | `-55,6 +61,14` |
| 4 | khối RiskFilter D3 đầu tiên: đổi hai ca (vùng giữa và sâu) theo khoảng đệm 0,5 °C | **SỬA** | `-71,10 +85,10` |
| 5 | cuối `main()`, ngay trước dòng in tổng: thêm các nhóm ca mới | **THÊM** | `-103,6 +117,105` |

```diff
--- a/tools/test-risk/test_risk.cpp
+++ b/tools/test-risk/test_risk.cpp
@@ -15,8 +15,8 @@
   RiskInput in;
   in.temp_ok = true;  in.temp_c = 26.0f;
   in.ph_ok = true;    in.ph = 7.0f;
-  in.turb_pct = 12.0f;
-  in.level_pct = 80.0f;
+  in.turb_ok = true;  in.turb_pct = 12.0f;
+  in.level_ok = true; in.level_pct = 80.0f;
   return in;
 }
 
@@ -31,6 +31,12 @@
   if (!ok) printf("      mong đợi risk=%d flags=[%s]\n", wantRisk, wantFlags);
 }
 
+static void expectTrue(bool ok, const char* name) {
+  total++;
+  if (!ok) failures++;
+  printf("%s  %s\n", ok ? "PASS" : "FAIL", name);
+}
+
 int main() {
   const RiskThresholds th = riskDefaultThresholds();
   RiskInput in;
@@ -55,6 +61,14 @@
   in = normalInput(); in.level_pct = 10.0f;              check("Mực nước 10", riskEvaluate(in, th), 2, "level_low");
   in = normalInput(); in.temp_ok = false;                check("Cảm biến nhiệt độ lỗi", riskEvaluate(in, th), 1, "sensor_temp");
   in = normalInput(); in.ph_ok = false;                  check("Cảm biến pH lỗi", riskEvaluate(in, th), 1, "sensor_ph");
+  in = normalInput(); in.turb_ok = false;                check("Cảm biến độ đục lỗi", riskEvaluate(in, th), 1, "sensor_turb");
+  in = normalInput(); in.level_ok = false;               check("Cảm biến mực nước lỗi", riskEvaluate(in, th), 1, "sensor_level");
+  in = normalInput(); in.temp_ok = false; in.ph_ok = false; in.turb_ok = false; in.level_ok = false;
+                                                         check("4 cảm biến cùng lỗi (4 cờ mức 1)", riskEvaluate(in, th), 3, "sensor_temp,sensor_ph,sensor_turb,sensor_level");
+  in = normalInput(); in.turb_ok = false; in.turb_pct = 99.0f;
+                                                         check("Độ đục lỗi: bỏ qua số đo, không báo turb_high", riskEvaluate(in, th), 1, "sensor_turb");
+  in = normalInput(); in.level_ok = false; in.level_pct = 0.0f;
+                                                         check("Mực nước lỗi: bỏ qua số đo, không báo level_low", riskEvaluate(in, th), 1, "sensor_level");
   in = normalInput(); in.temp_c = 30; in.turb_pct = 35; in.level_pct = 25;
                                                          check("3 cờ mức 1 (30 °C, 35%, 25%)", riskEvaluate(in, th), 2, "temp_high,turb_high,level_low");
   in.ph = 8.2f;                                          check("4 cờ mức 1 (thêm pH 8,2)", riskEvaluate(in, th), 3, "temp_high,ph_high,turb_high,level_low");
@@ -71,10 +85,10 @@
     check("Lần 1 nhiệt độ 30: chưa nâng", f.update(hot, th), 0, "");
     check("Lần 2 nhiệt độ 30: chưa nâng", f.update(hot, th), 0, "");
     check("Lần 3 nhiệt độ 30: nâng lên mức 1", f.update(hot, th), 1, "temp_high");
-    RiskInput near = normalInput(); near.temp_c = 28.9f;      // đã dưới 29 nhưng chưa dưới 28,8
-    check("Về 28,9 (vùng giữa): giữ mức 1", f.update(near, th), 1, "temp_high");
-    RiskInput deep = normalInput(); deep.temp_c = 28.7f;      // dưới 29 - 0,2
-    check("Về 28,7 (sâu trong ngưỡng): hạ về 0", f.update(deep, th), 0, "");
+    RiskInput near = normalInput(); near.temp_c = 28.7f;      // đã dưới 29 nhưng chưa tới 28,5 (29 - 0,5)
+    check("Về 28,7 (vùng giữa): giữ mức 1", f.update(near, th), 1, "temp_high");
+    RiskInput edge = normalInput(); edge.temp_c = 28.5f;      // đúng 28,5: hạ (so sánh nghiêm ngặt)
+    check("Về đúng 28,5 (29 - 0,5): hạ về 0", f.update(edge, th), 0, "");
     (void)ok;
   }
   {
@@ -103,6 +117,105 @@
     check("Chuỗi mức 3, 2, 3: nâng lên mức thấp nhất (2)", f.update(a, th), 2, "temp_high");
   }
 
+  // ---- Khoảng đệm khi hạ mức: 0,5 °C, 0,3 pH, 5 điểm % (nhóm chốt, xem documentation/fix-plan.md) ----
+  printf("\n=== Quy tắc hạ mức (khoảng đệm 0,5 / 0,3 / 5) ===\n");
+  {
+    RiskFilter f; RiskInput x = normalInput();
+    x.temp_c = 30.0f; f.update(x, th); f.update(x, th); f.update(x, th);
+    x.temp_c = 28.6f; check("Nhiệt độ cao: về 28,6 (chưa tới 28,5) giữ mức 1", f.update(x, th), 1, "temp_high");
+    x.temp_c = 28.5f; check("Nhiệt độ cao: về 28,5 hạ về 0", f.update(x, th), 0, "");
+  }
+  {
+    RiskFilter f; RiskInput x = normalInput();
+    x.temp_c = 22.0f; f.update(x, th); f.update(x, th); f.update(x, th);
+    x.temp_c = 23.4f; check("Nhiệt độ thấp: về 23,4 (chưa tới 23,5) giữ mức 1", f.update(x, th), 1, "temp_low");
+    x.temp_c = 23.5f; check("Nhiệt độ thấp: về 23,5 hạ về 0", f.update(x, th), 0, "");
+  }
+  {
+    RiskFilter f; RiskInput x = normalInput();
+    x.ph = 8.2f; f.update(x, th); f.update(x, th); f.update(x, th);
+    x.ph = 7.8f; check("pH cao: về 7,8 (chưa tới 7,7) giữ mức 1", f.update(x, th), 1, "ph_high");
+    x.ph = 7.7f; check("pH cao: về 7,7 hạ về 0", f.update(x, th), 0, "");
+  }
+  {
+    RiskFilter f; RiskInput x = normalInput();
+    x.turb_pct = 35.0f; f.update(x, th); f.update(x, th); f.update(x, th);
+    x.turb_pct = 26.0f; check("Độ đục: về 26 (chưa tới 25) giữ mức 1", f.update(x, th), 1, "turb_high");
+    x.turb_pct = 25.0f; check("Độ đục: về 25 hạ về 0", f.update(x, th), 0, "");
+  }
+  {
+    RiskFilter f; RiskInput x = normalInput();
+    x.level_pct = 25.0f; f.update(x, th); f.update(x, th); f.update(x, th);
+    x.level_pct = 34.0f; check("Mực nước: lên 34 (chưa tới 35) giữ mức 1", f.update(x, th), 1, "level_low");
+    x.level_pct = 35.0f; check("Mực nước: lên 35 hạ về 0", f.update(x, th), 0, "");
+  }
+  {
+    RiskFilter f; RiskInput x = normalInput();
+    x.level_pct = 10.0f; f.update(x, th); f.update(x, th); f.update(x, th);
+    x.level_pct = 19.0f; check("Mực nước mức 2: lên 19 (chưa tới 20) giữ mức 2", f.update(x, th), 2, "level_low");
+    x.level_pct = 20.0f; check("Mực nước mức 2: lên 20 hạ về mức 1", f.update(x, th), 1, "level_low");
+  }
+
+  // ---- #9 hướng A: khoảng đệm cho biên nguy hiểm ----
+  printf("\n=== Khoảng đệm ở biên nguy hiểm (#9) ===\n");
+  {
+    RiskFilter f; RiskInput x = normalInput();
+    x.temp_c = 33.1f; f.update(x, th); f.update(x, th);
+    check("Nhiệt độ 33,1 lần 3: nâng lên mức 3", f.update(x, th), 3, "temp_high");
+    x.temp_c = 32.98f; check("Về 32,98: vẫn mức 3 (không nhấp nháy)", f.update(x, th), 3, "temp_high");
+    x.temp_c = 32.6f;  check("Về 32,6: vẫn mức 3", f.update(x, th), 3, "temp_high");
+    x.temp_c = 32.5f;  check("Về đúng 32,5 (33 - 0,5): hạ xuống mức 2", f.update(x, th), 2, "temp_high");
+  }
+  {
+    RiskFilter f; RiskInput x = normalInput();
+    x.temp_c = 17.9f; f.update(x, th); f.update(x, th);
+    check("Nhiệt độ 17,9 lần 3: nâng lên mức 3", f.update(x, th), 3, "temp_low");
+    x.temp_c = 18.4f; check("Lên 18,4: vẫn mức 3", f.update(x, th), 3, "temp_low");
+    x.temp_c = 18.5f; check("Lên đúng 18,5 (18 + 0,5): hạ xuống mức 2", f.update(x, th), 2, "temp_low");
+  }
+  {
+    RiskFilter f; RiskInput x = normalInput();
+    x.ph = 8.6f; f.update(x, th); f.update(x, th);
+    check("pH 8,6 lần 3: nâng lên mức 2", f.update(x, th), 2, "ph_high");
+    x.ph = 8.3f; check("Về 8,3: vẫn mức 2", f.update(x, th), 2, "ph_high");
+    x.ph = 8.2f; check("Về đúng 8,2 (8,5 - 0,3): hạ xuống mức 1", f.update(x, th), 1, "ph_high");
+  }
+  {
+    RiskFilter f; RiskInput x = normalInput();
+    x.ph = 4.9f; f.update(x, th); f.update(x, th);
+    check("pH 4,9 lần 3: nâng lên mức 2", f.update(x, th), 2, "ph_low");
+    x.ph = 5.2f; check("Lên 5,2: vẫn mức 2", f.update(x, th), 2, "ph_low");
+    x.ph = 5.3f; check("Lên đúng 5,3 (5,0 + 0,3): hạ xuống mức 1", f.update(x, th), 1, "ph_low");
+  }
+
+  // ---- Cờ lỗi cảm biến mới đi qua bộ lọc (cần 3 lần liên tiếp) ----
+  printf("\n=== Lỗi cảm biến độ đục và mực nước qua bộ lọc ===\n");
+  {
+    RiskFilter f; RiskInput bad = normalInput(); bad.turb_ok = false; bad.level_ok = false;
+    f.update(bad, th); f.update(bad, th);
+    check("Hai cảm biến lỗi 2 lần: chưa đặt cờ", f.update(normalInput(), th), 0, "");
+    f.update(bad, th); f.update(bad, th);
+    check("Lỗi 3 lần liên tiếp: đặt sensor_turb và sensor_level", f.update(bad, th), 1, "sensor_turb,sensor_level");
+    check("Hết lỗi: bỏ cờ", f.update(normalInput(), th), 0, "");
+  }
+
+  // ---- Bảo vệ khi ngưỡng hẹp bất thường: khoảng đệm không làm ngưỡng thu hẹp chồng nhau ----
+  printf("\n=== riskTighten: biên và bảo vệ ===\n");
+  {
+    RiskThresholds d = riskDefaultThresholds();
+    expectTrue(d.temp_danger_min == 18.0f && d.temp_danger_max == 33.0f && d.ph_danger_min == 5.0f && d.ph_danger_max == 8.5f,
+               "riskDefaultThresholds có đủ 4 biên nguy hiểm");
+    RiskThresholds t = riskTighten(d);
+    expectTrue(t.temp_danger_min == 18.5f && t.temp_danger_max == 32.5f && t.temp_min == 23.5f && t.temp_max == 28.5f,
+               "riskTighten mặc định: nhiệt độ 23,5 đến 28,5; biên nguy hiểm 18,5 đến 32,5");
+    expectTrue(t.turb_max_pct == 25.0f && t.level_min_pct == 35.0f && t.level_danger_pct == 20.0f,
+               "riskTighten mặc định: độ đục 25; mực nước 35 và 20");
+    RiskThresholds nar = d; nar.temp_min = 26.0f; nar.temp_max = 27.0f; nar.turb_max_pct = 4.0f; nar.level_min_pct = 98.0f; nar.level_danger_pct = 97.0f;
+    RiskThresholds tn = riskTighten(nar);
+    expectTrue(tn.temp_min < tn.temp_max && tn.turb_max_pct > 0.0f && tn.level_min_pct <= 100.0f && tn.level_danger_pct <= tn.level_min_pct,
+               "Ngưỡng hẹp: ngưỡng thu hẹp không chồng nhau, độ đục > 0, mực nước <= 100");
+  }
+
   printf("\nTổng: %d ca, sai %d\n", total, failures);
   if (failures == 0) printf("TẤT CẢ ĐẠT\n");
   return failures == 0 ? 0 : 1;
```

#### Tệp: `tools/test-sensors-alarm/test_sensors_alarm.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | sau ca `Điện áp NaN`: thêm ca `sensorsLevelOk()` | **THÊM** | `-99,6 +99,20` |
| 2 | khối HAL giả bình thường: hai ca độ đục và mực nước | **SỬA** | `-129,8 +143,8` |
| 3 | sau ca `DS18B20 đọc đúng 85,0`: thêm khối lỗi độ đục và mực nước | **THÊM** | `-164,6 +178,18` |

```diff
--- a/tools/test-sensors-alarm/test_sensors_alarm.cpp
+++ b/tools/test-sensors-alarm/test_sensors_alarm.cpp
@@ -99,6 +99,20 @@
   expect(!sensorsAdcVoltOk(5.3f), "Điện áp 5,3 V: lỗi");
   expect(!sensorsAdcVoltOk(0.0f / 0.0f), "Điện áp NaN: lỗi");
 
+  { float ok8[8] = {1250, 1252, 1249, 1251, 1250, 1248, 1253, 1250};
+    expect(sensorsLevelOk(ok8, 8), "Mực nước: 8 mẫu ổn định (1250 ± 3): hợp lệ");
+    float dry8[8] = {0, 0, 0, 0, 0, 0, 0, 0};
+    expect(sensorsLevelOk(dry8, 8), "Mực nước: khô (0): vẫn hợp lệ, báo level_low chứ không phải lỗi cảm biến");
+    float sat8[8] = {4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095};
+    expect(!sensorsLevelOk(sat8, 8), "Mực nước: kẹt trần 4095: lỗi");
+    float jump8[8] = {1250, 1250, 1250, 1250, 1250, 1250, 1250, 1650};
+    expect(!sensorsLevelOk(jump8, 8), "Mực nước: một vòng đo nhảy hơn LEVEL_SPREAD_MAX: lỗi");
+    float edge8[8] = {1250, 1250, 1250, 1250, 1250, 1250, 1250, 1550};
+    expect(sensorsLevelOk(edge8, 8), "Mực nước: chênh đúng 300: chưa vượt, hợp lệ");
+    float nan8[2] = {1250.0f, 0.0f / 0.0f};
+    expect(!sensorsLevelOk(nan8, 2), "Mực nước: có mẫu NaN: lỗi");
+    expect(!sensorsLevelOk(ok8, 0), "Mực nước: không có mẫu nào: lỗi"); }
+
   { SensorCal c = testCal();
     expect(near(sensorsPhFromVolt(2.5f, c), 6.86f, 0.001f), "pH tại V686 = 6,86");
     expect(near(sensorsPhFromVolt(3.0f, c), 4.01f, 0.001f), "pH tại V401 = 4,01");
@@ -129,8 +143,8 @@
     expect(firstReady <= 1000, "Có đủ số liệu đầu tiên trong vòng 1 giây");
     expect(v.temp_ok && near(v.temp_c, 26.5f, 0.01f), "Nhiệt độ đọc được 26,5 °C");
     expect(v.ph_ok && near(v.ph, 6.86f, 0.01f) && near(v.ph_v, 2.5f, 0.001f), "pH 6,86 (2,50 V)");
-    expect(near(v.turb_v, 3.25f, 0.001f) && near(v.turb_pct, 50.0f, 0.01f), "Độ đục 50 % (3,25 V)");
-    expect(v.level_raw == 1250 && near(v.level_pct, 50.0f, 0.01f), "Mực nước 50 % (thô 1250)");
+    expect(v.turb_ok && near(v.turb_v, 3.25f, 0.001f) && near(v.turb_pct, 50.0f, 0.01f), "Độ đục 50 % (3,25 V), turb_ok = true");
+    expect(v.level_ok && v.level_raw == 1250 && near(v.level_pct, 50.0f, 0.01f), "Mực nước 50 % (thô 1250), level_ok = true");
     expect(hal.tempRequests >= 6 && hal.tempRequests <= 7, "Nhiệt độ: khoảng 2 giây một lần (6 đến 7 lần trong 12 giây)");
     expect(hal.phReads == 3 * ADC_SAMPLES && hal.turbReads == 3 * ADC_SAMPLES, "pH và độ đục: 3 vòng x 32 mẫu (5 giây một vòng)");
     expect(hal.levelReads == 3 * LEVEL_SAMPLES && hal.powerOns == 3, "Mực nước: 3 vòng, mỗi vòng bật nguồn một lần");
@@ -164,6 +178,18 @@
     expect(!hub.values().temp_ok, "DS18B20 đọc đúng 85,0: coi là lỗi (D4)");
   }
   {
+    FakeHal hal; hal.turbMv = 0.0f; hal.levelRawVal = 4095;      // độ đục 0 V, mực nước kẹt trần
+    SensorHub hub(hal, testCal());
+    runHub(hal, hub, 0, 1500);
+    const SensorValues& v = hub.values();
+    expect(!v.turb_ok, "Độ đục 0 V (dây đứt hoặc chập): turb_ok = false");
+    expect(!v.level_ok, "Mực nước kẹt trần 4095: level_ok = false");
+    RiskInput ri = sensorsToRiskInput(v);
+    RiskResult rr = riskEvaluate(ri, riskDefaultThresholds());
+    char flags[96]; riskFlagsCsv(rr.levels, flags, sizeof(flags));
+    expect(rr.risk == 1 && strcmp(flags, "sensor_turb,sensor_level") == 0, "Độ đục và mực nước lỗi: 2 cờ mức 1 (chưa đủ 3 cờ) nên risk 1");
+  }
+  {
     FakeHal hal; SensorHub hub(hal, testCal());
     uint32_t start = 0xFFFFFC00u;                 // millis() sắp quay vòng
     uint32_t fr = runHub(hal, hub, start, 3000);
```

#### Tệp: `tools/test-store-feeder/test_store_feeder.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | đầu `main()`, sau ca `Cấu hình mặc định hợp lệ`: thêm ca biên nguy hiểm | **THÊM** | `-44,6 +44,9` |

```diff
--- a/tools/test-store-feeder/test_store_feeder.cpp
+++ b/tools/test-store-feeder/test_store_feeder.cpp
@@ -44,6 +44,9 @@
   printf("=== store: kiểm tra cấu hình ===\n");
   AppConfig c = configDefaults();
   expect(configValidate(c, err, sizeof(err)), "Cấu hình mặc định hợp lệ");
+  { RiskThresholds th = configThresholds(c);
+    expect(th.temp_danger_min == 18.0f && th.temp_danger_max == 33.0f && th.ph_danger_min == 5.0f && th.ph_danger_max == 8.5f,
+           "configThresholds: có đủ 4 biên nguy hiểm"); }
 
   c = configDefaults(); c.temp_min = 18.0f; c.temp_max = 33.0f;
   expect(configValidate(c, err, sizeof(err)), "Đúng biên nhiệt độ 18 và 33: hợp lệ");
```

**Kiểm tra sau Phase 1** (chạy theo mục 4.1): `test_risk` **72 ca, 0 sai**; `test_sensors_alarm` **93 ca, 0 sai**; `test_store_feeder` **77 ca, 0 sai**; `node --check docs/app.js` và `py_compile` đạt; kiểm tra cú pháp bằng tệp giả (mục 4.4) đạt cho `net.cpp` và `fish_tank.ino`.

**Commit gợi ý:** `risk: them co loi cam bien do duc va muc nuoc, khoang dem 0,5/0,3/5 va biên nguy hiem`


---

## Phase 2: Lịch cho ăn mặc định (#1)

**Điều kiện:** kiểm tra sau Phase 1 đạt.

### Bước 2.1 Hằng số và mặc định trong firmware

Mặc định khi chưa có config: 3 mốc 08:00, 14:00, 18:00 (khớp tối đa 3 mốc và hạn mức 4 lần mỗi 24 giờ).

#### Tệp: `firmware/fish_tank/config.h`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | sau `FEED_SCHEDULE_GRACE_S`: thêm 4 hằng số lịch mặc định | **THÊM** | `-135,6 +135,10` |

```diff
--- a/firmware/fish_tank/config.h
+++ b/firmware/fish_tank/config.h
@@ -135,6 +135,10 @@
 #define FEED_WINDOW_S           86400UL
 #define FEED_MAX_TIMES          3          // D6: tối đa 3 mốc trong lịch (chừa 1 lần cho "Cho ăn ngay")
 #define FEED_SCHEDULE_GRACE_S   300UL      // D6: mỗi mốc chạy trong cửa sổ khoảng 5 phút sau giờ đặt
+#define DEF_FEED_COUNT          3          // lịch mặc định khi chưa có config (nhóm chốt, xem documentation/fix-plan.md): 08:00, 14:00, 18:00
+#define DEF_FEED_MIN_0          (8 * 60)
+#define DEF_FEED_MIN_1          (14 * 60)
+#define DEF_FEED_MIN_2          (18 * 60)
 
 // ---------------------------------------------------------------------------
 // Còi (E6, D9)
```

#### Tệp: `firmware/fish_tank/store.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | `configDefaults()`: dòng `c.feed_count = 0;` | **SỬA** | `-16,7 +16,10` |

```diff
--- a/firmware/fish_tank/store.cpp
+++ b/firmware/fish_tank/store.cpp
@@ -16,7 +16,10 @@
   c.buzzer_on = DEF_BUZZER_ON;
   c.mute_min = DEF_MUTE_MIN;
   c.feed_ms = FEED_MS_DEFAULT;
-  c.feed_count = 0;
+  c.feed_count = DEF_FEED_COUNT;                       // mặc định 08:00, 14:00, 18:00 (nhóm chốt, xem documentation/fix-plan.md)
+  c.feed_min[0] = DEF_FEED_MIN_0;
+  c.feed_min[1] = DEF_FEED_MIN_1;
+  c.feed_min[2] = DEF_FEED_MIN_2;
   return c;
 }
 
```

### Bước 2.2 Web, ESP32 giả và mẫu MQTTX

`config-default.json` trước đây ghi ngưỡng khác mặc định của firmware (24 đến 30 °C, độ đục 40) và 2 mốc cho ăn; đồng bộ lại để lỡ publish mẫu này không làm lệch cấu hình.

#### Tệp: `docs/app.js`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | `DEFAULT_CFG.feed_times` | **SỬA** | `-31,7 +31,7` |

```diff
--- a/docs/app.js
+++ b/docs/app.js
@@ -31,7 +31,7 @@
 const DEFAULT_CFG = {
     temp_min: 23, temp_max: 29, ph_min: 6.0, ph_max: 8.0,
     turb_max_pct: 30, level_min_pct: 30, level_danger_pct: 15,
-    mute_min: 30, feed_ms: 600, buzzer_on: true, feed_times: ["08:00", "18:00"],
+    mute_min: 30, feed_ms: 600, buzzer_on: true, feed_times: ["08:00", "14:00", "18:00"],
 };
 
 const $ = (id) => document.getElementById(id);
```

#### Tệp: `docs/index.html`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | ô nhập `#s_times`: giá trị mặc định | **SỬA** | `-104,7 +104,7` |

```diff
--- a/docs/index.html
+++ b/docs/index.html
@@ -104,7 +104,7 @@
         <label>Tắt tiếng còi (phút)<input id="s_mute" type="number" step="1" min="0" max="240" value="30"></label>
         <label>Thời gian quay một phần ăn (ms)<input id="s_feedms" type="number" step="10" min="100" max="3000"
             value="600"></label>
-        <label>Giờ cho ăn tự động (tối đa 3, ngăn cách bằng dấu phẩy)<input id="s_times" value="08:00, 18:00"></label>
+        <label>Giờ cho ăn tự động (tối đa 3, ngăn cách bằng dấu phẩy)<input id="s_times" value="08:00, 14:00, 18:00"></label>
         <label class="check"><input id="s_buzzer" type="checkbox" checked> Bật còi cảnh báo</label>
       </div>
       <p class="hint">Biên an toàn: nhiệt độ 18–33 °C, pH 5,0–8,5. Mức 2 của nhiệt độ và độ đục tự đi theo ngưỡng mức 1
```

#### Tệp: `tools/fake-esp32/fake_esp32.py`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | `DEFAULT_CONFIG["feed_times"]` | **SỬA** | `-53,7 +53,7` |

```diff
--- a/tools/fake-esp32/fake_esp32.py
+++ b/tools/fake-esp32/fake_esp32.py
@@ -53,7 +53,7 @@
     "level_min_pct": 30.0, "level_danger_pct": 15.0,
     "buzzer_on": True, "mute_min": 30,
     "feed_ms": 600,
-    "feed_times": [],
+    "feed_times": ["08:00", "14:00", "18:00"],   # mặc định như firmware (nhóm chốt, xem documentation/fix-plan.md)
 }
 
 # ----------------------------------------------------------------------------
```

#### Tệp: `tools/mqtt-samples/config-default.json`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | toàn bộ nội dung (một dòng JSON): cho khớp mặc định của firmware | **SỬA** | `-1 +1` |

```diff
--- a/tools/mqtt-samples/config-default.json
+++ b/tools/mqtt-samples/config-default.json
@@ -1 +1 @@
-{"v": 1, "temp_min": 24, "temp_max": 30, "ph_min": 6.5, "ph_max": 8.0, "turb_max_pct": 40, "level_min_pct": 30, "level_danger_pct": 15, "buzzer_on": true, "mute_min": 30, "feed_ms": 600, "feed_times": ["08:00", "18:00"]}
+{"v": 1, "temp_min": 23, "temp_max": 29, "ph_min": 6.0, "ph_max": 8.0, "turb_max_pct": 30, "level_min_pct": 30, "level_danger_pct": 15, "buzzer_on": true, "mute_min": 30, "feed_ms": 600, "feed_times": ["08:00", "14:00", "18:00"]}
```

### Bước 2.3 Bộ ca thử

Đổi kỳ vọng mặc định và thêm ca cho lịch 3 mốc.

#### Tệp: `tools/test-store-feeder/test_store_feeder.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | đầu `main()`: thêm ca lịch mặc định | **THÊM** | `-44,6 +44,8` |
| 2 | hai ca `configParseFeedTimes` (mốc trùng, mốc sai dạng): đổi kỳ vọng `feed_count == 0` thành `DEF_FEED_COUNT` | **SỬA** | `-94,13 +96,13` |
| 3 | khối `feeder: lịch tự động (D6)`: thêm khối ca lịch mặc định | **THÊM** | `-164,6 +166,15` |

```diff
--- a/tools/test-store-feeder/test_store_feeder.cpp
+++ b/tools/test-store-feeder/test_store_feeder.cpp
@@ -44,6 +44,8 @@
   printf("=== store: kiểm tra cấu hình ===\n");
   AppConfig c = configDefaults();
   expect(configValidate(c, err, sizeof(err)), "Cấu hình mặc định hợp lệ");
+  expect(c.feed_count == 3 && c.feed_min[0] == 480 && c.feed_min[1] == 840 && c.feed_min[2] == 1080,
+         "Lịch cho ăn mặc định: 3 mốc 08:00, 14:00, 18:00");
   { RiskThresholds th = configThresholds(c);
     expect(th.temp_danger_min == 18.0f && th.temp_danger_max == 33.0f && th.ph_danger_min == 5.0f && th.ph_danger_max == 8.5f,
            "configThresholds: có đủ 4 biên nguy hiểm"); }
@@ -94,13 +96,13 @@
            c.feed_min[0] == 480 && c.feed_min[1] == 1080, "Hai mốc: tự sắp tăng dần (08:00, 18:00)"); }
   { const char* t[] = {"08:00", "08:00"};
     AppConfig d = configDefaults();
-    expect(!configParseFeedTimes(t, 2, d, err, sizeof(err)) && d.feed_count == 0, "Hai mốc trùng: từ chối, không đổi cấu hình"); }
+    expect(!configParseFeedTimes(t, 2, d, err, sizeof(err)) && d.feed_count == DEF_FEED_COUNT, "Hai mốc trùng: từ chối, không đổi cấu hình"); }
   { const char* t[] = {"06:00", "12:00", "18:00", "21:00"};
     AppConfig d = configDefaults();
     expect(!configParseFeedTimes(t, 4, d, err, sizeof(err)), "4 mốc (D6 chỉ cho 3): từ chối"); }
   { const char* t[] = {"08:00", "9:30"};
     AppConfig d = configDefaults();
-    expect(!configParseFeedTimes(t, 2, d, err, sizeof(err)) && d.feed_count == 0, "Một mốc sai dạng: từ chối cả danh sách"); }
+    expect(!configParseFeedTimes(t, 2, d, err, sizeof(err)) && d.feed_count == DEF_FEED_COUNT, "Một mốc sai dạng: từ chối cả danh sách"); }
 
   printf("\n=== feeder: tiện ích ===\n");
   expect(feedClampPortions(0) == 1 && feedClampPortions(-3) == 1, "Số phần 0 hoặc âm: kẹp về 1");
@@ -164,6 +166,15 @@
     expect(p.scheduleDue(slots, 2, at(18, 1)) == 1, "Mốc 18:00 vẫn chạy bình thường");
   }
   {
+    AppConfig dc = configDefaults();                 // lịch mặc định 08:00, 14:00, 18:00
+    FeedPolicy p;
+    expect(p.scheduleDue(dc.feed_min, dc.feed_count, at(8, 0, 0)) == 0, "Lịch mặc định: 08:00 là mốc 0");
+    expect(p.scheduleDue(dc.feed_min, dc.feed_count, at(13, 59, 59)) == -1, "Lịch mặc định: 13:59:59 chưa tới mốc 14:00");
+    expect(p.scheduleDue(dc.feed_min, dc.feed_count, at(14, 0, 0)) == 1, "Lịch mặc định: 14:00 là mốc 1");
+    expect(p.scheduleDue(dc.feed_min, dc.feed_count, at(18, 4, 59)) == 2, "Lịch mặc định: 18:04:59 vẫn trong cửa sổ mốc 2");
+    expect(p.scheduleDue(dc.feed_min, dc.feed_count, at(18, 5, 0)) == -1, "Lịch mặc định: 18:05:00 hết cửa sổ");
+  }
+  {
     uint16_t slots[1] = {480};
     FeedPolicy p;
     p.commit(at(7, 58));                   // cho ăn tay trước giờ đặt
```

**Kiểm tra sau Phase 2:** `test_store_feeder` **83 ca, 0 sai** (hai bộ còn lại không đổi: 72 và 93); `node --check docs/app.js`; `py_compile`. Lưu ý quan trọng cho nhóm: broker EMQX có thể còn config retained cũ (08:00 và 18:00) và nó sẽ ghi đè mặc định khi ESP32 nối; sau khi nạp firmware, bấm **Lưu** ở phần Cài đặt trên web, hoặc dùng MQTTX publish `tools/mqtt-samples/config-default.json` (retain bật) vào `fishtank/tank01/config`. Đây là việc của nhóm, Agent không có mật khẩu và không làm.

**Commit gợi ý:** `feed: lich cho an mac dinh 08:00, 14:00, 18:00 (firmware, web, esp32 gia, mau MQTTX)`


---

## Phase 3: Còi mức 3 và tắt tiếng từ xa (D9, #4, #5)

**Điều kiện:** kiểm tra sau Phase 2 đạt.

### Bước 3.1 Còi mức 3: 3 tiếng mỗi 5 giây

Mức 3 kêu to nhất (duty 50%): 3 tiếng 200 ms cách nhau 200 ms, lặp mỗi 5 giây (`BUZZ_L3_PERIOD_MS`). Cách hoạt động của tắt tiếng không đổi (đã có sẵn trong lớp `Alarm`): im 30 phút, tự hủy khi risk tăng hoặc về 0, risk giảm thì vẫn im.

#### Tệp: `firmware/fish_tank/config.h`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | sau `TOPIC_CMD_FEED`: thêm `TOPIC_CMD_MUTE` | **THÊM** | `-27,6 +27,7` |
| 2 | khối `BUZZ_*`: thay dòng chú thích "mức 3 liên tục" bằng `BUZZ_L3_PERIOD_MS` | **SỬA** | `-149,7 +150,7` |

```diff
--- a/firmware/fish_tank/config.h
+++ b/firmware/fish_tank/config.h
@@ -27,6 +27,7 @@
 #define TOPIC_STATUS        TOPIC_BASE "status"      // online / offline, retained (Last Will QoS 1)
 #define TOPIC_EVENT         TOPIC_BASE "event"       // QoS 0
 #define TOPIC_CMD_FEED      TOPIC_BASE "cmd/feed"    // QoS 1 (ESP32 subscribe)
+#define TOPIC_CMD_MUTE      TOPIC_BASE "cmd/mute"    // QoS 1 (ESP32 subscribe): {"action":"mute"} hoặc {"action":"unmute"}
 #define TOPIC_CMD_ALL       TOPIC_BASE "cmd/#"
 #define TOPIC_CONFIG        TOPIC_BASE "config"      // QoS 1, retained (ESP32 subscribe)
 
@@ -149,7 +150,7 @@
 #define BUZZ_BEEP_MS            200
 #define BUZZ_L1_PERIOD_MS       60000UL    // mức 1: 1 tiếng ngắn mỗi 60 giây
 #define BUZZ_L2_PERIOD_MS       30000UL    // mức 2: 2 tiếng mỗi 30 giây (cách nhau BUZZ_BEEP_MS)
-                                           // mức 3: kêu to nhất liên tục
+#define BUZZ_L3_PERIOD_MS       5000UL     // mức 3: 3 tiếng to nhất liền nhau mỗi 5 giây (mỗi tiếng BUZZ_BEEP_MS, cách nhau BUZZ_BEEP_MS)
 #define BUZZ_LEDC_CHANNEL       4          // chỉ dùng cho core 2.x (core 3.x tự cấp kênh); tránh kênh 0 của servo
 #define BUZZ_LEDC_BITS          8          // độ phân giải PWM
 #define BUZZ_ACTIVE_LOW         0          // đặt 1 nếu còi kêu liên tục khi chưa phát (đảo logic, mục E5)
```

#### Tệp: `firmware/fish_tank/alarm.h`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | chú thích đầu tệp: dòng `Mức 3` | **SỬA** | `-4,7 +4,7` |

```diff
--- a/firmware/fish_tank/alarm.h
+++ b/firmware/fish_tank/alarm.h
@@ -4,7 +4,7 @@
 //   Mức 0: im lặng
 //   Mức 1: 1 tiếng ngắn nhỏ (BUZZ_BEEP_MS) mỗi 60 giây
 //   Mức 2: 2 tiếng nhỏ, cách nhau BUZZ_BEEP_MS, mỗi 30 giây
-//   Mức 3: kêu to nhất, liên tục
+//   Mức 3: 3 tiếng to nhất liền nhau (mỗi tiếng BUZZ_BEEP_MS, cách nhau BUZZ_BEEP_MS) mỗi BUZZ_L3_PERIOD_MS (5 giây)
 //
 // Lớp Alarm là máy trạng thái THUẦN: nhận (thời gian, risk) và trả về "còi bật hay tắt, độ rộng xung bao nhiêu %".
 // Phần ghi ra chân còi (ledc PWM) nằm cuối file, chỉ khi biên dịch cho ESP32.
```

#### Tệp: `firmware/fish_tank/alarm.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | hàm tính đầu ra của `Alarm::tick()`: nhánh `default` (mức 3) | **SỬA** | `-65,10 +65,14` |

```diff
--- a/firmware/fish_tank/alarm.cpp
+++ b/firmware/fish_tank/alarm.cpp
@@ -65,10 +65,14 @@
       o.dutyPct = BUZZ_DUTY_QUIET_PCT;
       break;
     }
-    default:                                     // mức 3: to nhất, liên tục
-      o.on = true;
+    default: {                                   // mức 3: to nhất, 3 tiếng liền nhau mỗi BUZZ_L3_PERIOD_MS (5 giây)
+      uint32_t phase = elapsed % BUZZ_L3_PERIOD_MS;
+      o.on = phase < BUZZ_BEEP_MS ||                                   // tiếng thứ nhất
+             (phase >= 2UL * BUZZ_BEEP_MS && phase < 3UL * BUZZ_BEEP_MS) ||   // tiếng thứ hai
+             (phase >= 4UL * BUZZ_BEEP_MS && phase < 5UL * BUZZ_BEEP_MS);     // tiếng thứ ba
       o.dutyPct = BUZZ_DUTY_LOUD_PCT;
       break;
+    }
   }
   return o;
 }
```

### Bước 3.2 Firmware nhận lệnh `cmd/mute` và báo trạng thái

Subscribe thêm `fishtank/tank01/cmd/mute`; telemetry có trường tùy chọn `muted`; event `mute`, `unmute`, `mute_rejected` (lý do `no_alarm` hoặc `disabled`). ACL trên EMQX không đổi vì `cmd/#` đã bao gồm topic này.

#### Tệp: `firmware/fish_tank/net.h`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | khai báo: `netTakeMuteCmd`, `netPublishTelemetry` (thêm tham số), `netPublishMute`, `netPublishMuteRejected` | **SỬA** | `-32,10 +32,13` |

```diff
--- a/firmware/fish_tank/net.h
+++ b/firmware/fish_tank/net.h
@@ -32,10 +32,13 @@
 // Lấy lệnh / config đang chờ. Trả true và điền out nếu có. Gọi lặp tới khi false.
 bool netTakeFeedCmd(NetFeedCmd& out);
 bool netTakeConfig(AppConfig& out);      // đã gộp vào cấu hình cũ và đã qua configValidate()
+bool netTakeMuteCmd(bool& wantMute);     // lệnh cmd/mute đang chờ: wantMute = true là "mute", false là "unmute"
 
 // Gửi lên broker. Trả false nếu chưa nối MQTT hoặc gửi lỗi.
-bool netPublishTelemetry(const SensorValues& s, const RiskResult& r);   // QoS 0, retained
+bool netPublishTelemetry(const SensorValues& s, const RiskResult& r, bool muted);   // QoS 0, retained; muted = còi đang tắt tiếng
 bool netPublishFeedRejected(const char* id, const char* reason);        // QoS 0
 bool netPublishFeedDone(const char* id, uint8_t portions);              // QoS 0
 bool netPublishAlarm(const RiskResult& r);                              // event "alarm" (level = risk, msg = các cờ); false nếu risk = 0
+bool netPublishMute(bool muted, uint16_t minutes);                     // event "mute" (kèm minutes) hoặc "unmute"; QoS 0
+bool netPublishMuteRejected(const char* reason);                        // event "mute_rejected": reason = no_alarm | disabled
 #endif
\ No newline at end of file
```

#### Tệp: `firmware/fish_tank/net.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | hằng `MUTE_QUEUE_LEN` | **THÊM** | `-26,6 +26,7` |
| 2 | biến hàng đợi lệnh tắt tiếng | **THÊM** | `-51,6 +52,9` |
| 3 | chú thích danh sách `type` của event | **SỬA** | `-79,7 +83,7` |
| 4 | thêm `netPublishMute()` và `netPublishMuteRejected()` trước `publishBoot()` | **THÊM** | `-119,6 +123,18` |
| 5 | `netPublishTelemetry()`: chữ ký hàm | **SỬA** | `-159,7 +175,7` |
| 6 | `netPublishTelemetry()`: chuỗi định dạng JSON và đối số | **SỬA** | `-174,8 +190,8` |
| 7 | trước `netTakeFeedCmd()`: thêm `handleMuteMessage()` và `netTakeMuteCmd()` | **THÊM** | `-215,6 +231,38` |
| 8 | `onMessage()`: thêm nhánh `cmd/mute` | **THÊM** | `-319,6 +367,7` |
| 9 | `tryConnectMqtt()`: subscribe thêm `TOPIC_CMD_MUTE` | **SỬA** | `-364,7 +413,8` |

```diff
--- a/firmware/fish_tank/net.cpp
+++ b/firmware/fish_tank/net.cpp
@@ -26,6 +26,7 @@
 #define MQTT_SOCKET_TO_S   5         // giây: giới hạn chờ khi nối và khi đọc
 #define TLS_HANDSHAKE_TO_S 8         // giây
 #define FEED_QUEUE_LEN     4
+#define MUTE_QUEUE_LEN     4
 
 // ---------------------------------------------------------------------------
 // Trạng thái (một thể hiện duy nhất)
@@ -51,6 +52,9 @@
 static NetFeedCmd s_feedQ[FEED_QUEUE_LEN];
 static uint8_t    s_feedHead = 0, s_feedCount = 0;
 
+static bool    s_muteQ[MUTE_QUEUE_LEN];     // true = mute, false = unmute
+static uint8_t s_muteHead = 0, s_muteCount = 0;
+
 static AppConfig s_pendingCfg;
 static bool      s_hasPendingCfg = false;
 static bool      s_cfgRejected = false;
@@ -79,7 +83,7 @@
 // ---------------------------------------------------------------------------
 // Gửi event (JSON dựng bằng ArduinoJson để id từ web được escape đúng)
 // Khớp onEvent() trong docs/app.js và fake_esp32.py: type, ts (giây), id, reason, portions, fw, level, msg (chuỗi).
-//   type: boot | feed_done | feed_rejected | alarm | config_rejected
+//   type: boot | feed_done | feed_rejected | alarm | config_rejected | mute | unmute | mute_rejected
 // ---------------------------------------------------------------------------
 static bool sendEventDoc(JsonDocument& doc) {
   uint32_t ts = netEpoch();
@@ -119,6 +123,18 @@
   return sendEventDoc(doc);
 }
 
+bool netPublishMute(bool muted, uint16_t minutes) {
+  if (!s_mqtt.connected()) return false;
+  JsonDocument doc;
+  doc["type"] = muted ? "mute" : "unmute";
+  if (muted) doc["minutes"] = minutes;
+  return sendEventDoc(doc);
+}
+
+bool netPublishMuteRejected(const char* reason) {
+  return publishEvent("mute_rejected", nullptr, reason, 0);
+}
+
 static bool publishBoot() {
   if (!s_mqtt.connected()) return false;
   JsonDocument doc;
@@ -159,7 +175,7 @@
   else snprintf(out, len, "null");
 }
 
-bool netPublishTelemetry(const SensorValues& s, const RiskResult& r) {
+bool netPublishTelemetry(const SensorValues& s, const RiskResult& r, bool muted) {
   if (!s_mqtt.connected()) return false;
   char temp[16], ph[16], turbV[16], turb[16], level[16], flags[96], tsPart[24];
   fmtNum(temp, sizeof(temp), s.temp_ok, s.temp_c, 1);
@@ -174,8 +190,8 @@
 
   char buf[320];
   int n = snprintf(buf, sizeof(buf),
-      "{\"v\":1,%s\"temp_c\":%s,\"ph\":%s,\"turb_v\":%s,\"turb_pct\":%s,\"level_pct\":%s,\"risk\":%u,\"flags\":%s,\"rssi\":%d,\"uptime_s\":%lu,\"fw\":\"%s\"}",
-      tsPart, temp, ph, turbV, turb, level, (unsigned)r.risk, flags, (int)WiFi.RSSI(),
+      "{\"v\":1,%s\"temp_c\":%s,\"ph\":%s,\"turb_v\":%s,\"turb_pct\":%s,\"level_pct\":%s,\"risk\":%u,\"muted\":%s,\"flags\":%s,\"rssi\":%d,\"uptime_s\":%lu,\"fw\":\"%s\"}",
+      tsPart, temp, ph, turbV, turb, level, (unsigned)r.risk, muted ? "true" : "false", flags, (int)WiFi.RSSI(),
       (unsigned long)(millis() / 1000UL), FW_VERSION);
   if (n <= 0 || n >= (int)sizeof(buf)) return false;
   return s_mqtt.publish(TOPIC_TELEMETRY, buf, true);
@@ -215,6 +231,38 @@
   s_feedCount++;
 }
 
+// ---------------------------------------------------------------------------
+// Nhận lệnh tắt tiếng: cmd/mute  {"action":"mute"} hoặc {"action":"unmute"}
+// Lưu ý: web KHÔNG được publish cmd/mute với retain (lệnh retained sẽ bị giao lại mỗi lần nối lại).
+// ---------------------------------------------------------------------------
+static void handleMuteMessage(const byte* payload, unsigned int len) {
+  if (len == 0) return;                                   // payload rỗng = xóa retained, bỏ qua
+  JsonDocument doc;
+  if (deserializeJson(doc, (const char*)payload, len)) {
+    Serial.println("[net] cmd/mute: JSON loi, bo qua");
+    return;
+  }
+  if (!doc.is<JsonObject>()) return;
+  JsonVariantConst av = doc.as<JsonObjectConst>()["action"];
+  if (!av.is<const char*>()) { Serial.println("[net] cmd/mute: thieu action, bo qua"); return; }
+  const char* a = av.as<const char*>();
+  bool want;
+  if (strcmp(a, "mute") == 0) want = true;
+  else if (strcmp(a, "unmute") == 0) want = false;
+  else { Serial.println("[net] cmd/mute: action la, bo qua"); return; }
+  if (s_muteCount >= MUTE_QUEUE_LEN) { Serial.println("[net] hang doi lenh tat tieng day, bo lenh"); return; }
+  s_muteQ[(s_muteHead + s_muteCount) % MUTE_QUEUE_LEN] = want;
+  s_muteCount++;
+}
+
+bool netTakeMuteCmd(bool& wantMute) {
+  if (s_muteCount == 0) return false;
+  wantMute = s_muteQ[s_muteHead];
+  s_muteHead = (s_muteHead + 1) % MUTE_QUEUE_LEN;
+  s_muteCount--;
+  return true;
+}
+
 bool netTakeFeedCmd(NetFeedCmd& out) {
   if (s_feedCount == 0) return false;
   out = s_feedQ[s_feedHead];
@@ -319,6 +367,7 @@
 // ---------------------------------------------------------------------------
 static void onMessage(char* topic, byte* payload, unsigned int len) {
   if (strcmp(topic, TOPIC_CMD_FEED) == 0) handleFeedMessage(payload, len);
+  else if (strcmp(topic, TOPIC_CMD_MUTE) == 0) handleMuteMessage(payload, len);
   else if (strcmp(topic, TOPIC_CONFIG) == 0) handleConfigMessage(payload, len);
 }
 
@@ -364,7 +413,8 @@
   if (ok) {
     bool s1 = s_mqtt.subscribe(TOPIC_CMD_FEED, 1);
     bool s2 = s_mqtt.subscribe(TOPIC_CONFIG, 1);
-    ok = s1 && s2;
+    bool s3 = s_mqtt.subscribe(TOPIC_CMD_MUTE, 1);
+    ok = s1 && s2 && s3;
     if (!ok) {
       Serial.println("[net] subscribe loi, ngat de thu lai");
       s_mqtt.disconnect();
```

#### Tệp: `firmware/fish_tank/fish_tank.ino`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | khai báo biến: thêm `g_prevMuted` | **THÊM** | `-30,6 +30,7` |
| 2 | trước `handleSchedule()`: thêm hàm `handleRemoteMute()` | **THÊM** | `-80,6 +81,29` |
| 3 | bước 3 (còi): sau `alarmApply(...)` thêm theo dõi trạng thái tắt tiếng | **THÊM** | `-189,6 +213,10` |
| 4 | bước 5b và 6: thêm bước 5c (nhận lệnh tắt tiếng); đổi lời gọi `netPublishTelemetry` thêm `g_alarm.muted()` | **SỬA** | `-215,9 +243,13` |

```diff
--- a/firmware/fish_tank/fish_tank.ino
+++ b/firmware/fish_tank/fish_tank.ino
@@ -30,6 +30,7 @@
 static uint32_t g_nextLogMs = 0;
 static uint8_t  g_prevRisk = 0;
 static uint16_t g_prevFlags = 0;         // mặt nạ bit các cờ đang có (để gửi telemetry ngay khi cờ đổi); 16 bit vì FL_COUNT = 10
+static bool     g_prevMuted = false;     // để gửi telemetry ngay khi trạng thái tắt tiếng đổi
 static uint32_t g_lastSchedRejectEpoch = 0;
 
 static bool due(uint32_t now, uint32_t target) {
@@ -80,6 +81,29 @@
   startFeed(cmd.id, feedClampPortions(cmd.portions), epoch, now);
 }
 
+// Lệnh tắt tiếng từ web. Alarm::mute() chỉ có hiệu lực khi đang có cảnh báo và mute_min > 0.
+static void handleRemoteMute(bool wantMute, uint32_t now) {
+  if (!wantMute) {
+    g_alarm.unmute();
+    Serial.println("[mute] da bo tat tieng");
+    netPublishMute(false, 0);
+    return;
+  }
+  if (g_cfg.mute_min == 0) {
+    Serial.println("[mute] tu choi: mute_min = 0");
+    netPublishMuteRejected("disabled");
+    return;
+  }
+  g_alarm.mute(now);
+  if (!g_alarm.muted()) {
+    Serial.println("[mute] tu choi: khong co canh bao");
+    netPublishMuteRejected("no_alarm");
+    return;
+  }
+  Serial.printf("[mute] tat tieng %u phut\n", (unsigned)g_cfg.mute_min);
+  netPublishMute(true, g_cfg.mute_min);
+}
+
 static void handleSchedule(uint32_t epoch, uint32_t now) {
   if (g_runner.busy() || g_cfg.feed_count == 0) return;
   if (!due(now, g_nextSchedMs)) return;
@@ -189,6 +213,10 @@
 
   // 3) Còi: chạy độc lập với mạng
   alarmApply(g_alarm.tick(now, g_risk.risk));
+  if (g_alarm.muted() != g_prevMuted) {  // tắt tiếng bật, tắt hoặc tự hủy: báo web ngay
+    g_prevMuted = g_alarm.muted();
+    g_nextTelemMs = now;
+  }
 
   // 4) Cho ăn: kết thúc lần quay, rồi lịch tự động
   bool feedDone = g_runner.tick(now);
@@ -215,9 +243,13 @@
   NetFeedCmd cmd;
   while (netTakeFeedCmd(cmd)) handleRemoteFeed(cmd, epoch, now);
 
+  // 5c) Lệnh tắt tiếng còi từ web (cmd/mute)
+  bool wantMute;
+  while (netTakeMuteCmd(wantMute)) handleRemoteMute(wantMute, now);
+
   // 6) Telemetry mỗi TELEMETRY_PERIOD_MS, và ngay khi risk hoặc cờ đổi (g_nextTelemMs = now ở trên)
   if (g_sensors.ready() && netMqttUp() && due(now, g_nextTelemMs)) {
-    bool ok = netPublishTelemetry(g_sensors.values(), g_risk);
+    bool ok = netPublishTelemetry(g_sensors.values(), g_risk, g_alarm.muted());
     g_nextTelemMs = now + (ok ? TELEMETRY_PERIOD_MS : 1000UL);
   }
 
```

### Bước 3.3 Web: nút tắt tiếng

Thanh nút chỉ hiện khi risk ≥ 1. Lệnh publish QoS 1, KHÔNG retain. Có chữ tiếng Việt cho `mute`, `unmute`, `mute_rejected` và cả `config_rejected` (#5). Tăng phiên bản cache của service worker.

#### Tệp: `docs/app.js`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | hằng `TOPIC`: thêm `mute` | **THÊM** | `-8,6 +8,7` |
| 2 | sau `FEED_WAIT_MS`: thêm `MUTE_WAIT_MS` | **THÊM** | `-22,6 +23,7` |
| 3 | sau `REASON_TEXT`: thêm `MUTE_REASON_TEXT` | **THÊM** | `-49,6 +51,10` |
| 4 | khai báo biến: thêm `pendingMute`, `muteTimer` | **THÊM** | `-66,6 +72,8` |
| 5 | trước `renderAlert()` thêm `renderMuteBar()`; trong `renderAlert()` thêm lời gọi nó | **THÊM** | `-336,11 +344,25` |
| 6 | `onEvent()`, nhánh `feed_done`: thêm chữ "(theo lịch)" | **SỬA** | `-526,7 +548,7` |
| 7 | `onEvent()`: thêm các loại event `mute`, `unmute`, `mute_rejected`, `config_rejected` | **THÊM** | `-537,6 +559,10` |
| 8 | `onEvent()`: xử lý phản hồi khi đang chờ lệnh tắt tiếng | **THÊM** | `-545,6 +571,12` |
| 9 | trước khối `CÀI ĐẶT`: thêm khối `TẮT TIẾNG CÒI` (`endMute`, `sendMute`, hai trình xử lý nút) | **THÊM** | `-641,6 +673,31` |

```diff
--- a/docs/app.js
+++ b/docs/app.js
@@ -8,6 +8,7 @@
     status: `fishtank/${TANK}/status`,
     event: `fishtank/${TANK}/event`,
     feed: `fishtank/${TANK}/cmd/feed`,
+    mute: `fishtank/${TANK}/cmd/mute`,
     config: `fishtank/${TANK}/config`,
 };
 
@@ -22,6 +23,7 @@
 const EVT_SHOW = 50;                       // số sự kiện hiển thị
 const ALARM_GAP_MS = { 1: 60000, 2: 30000, 3: 15000 };   // giãn cách tối thiểu giữa hai dòng cảnh báo CÙNG MỨC: mức 1 = 60 giây, mức 2 = 30 giây, mức 3 = 15 giây
 const FEED_WAIT_MS = 15000;                // chờ phản hồi cho ăn tối đa 15 giây
+const MUTE_WAIT_MS = 10000;                // chờ phản hồi tắt tiếng tối đa 10 giây
 const MAX_FEED_TIMES = 3;                  // D6 (đề xuất): tối đa 3 mốc, chừa 1 lần cho "Cho ăn ngay". Đổi thành 4 nếu nhóm chốt 4.
 
 const LIMIT = { tempMin: 18, tempMax: 33, phMin: 5.0, phMax: 8.5 };   // biên nguy hiểm (mục E6, E8)
@@ -49,6 +51,10 @@
     busy: "đang cho ăn",
     no_time: "thiết bị chưa đồng bộ giờ, thử lại sau ít phút",
 };
+const MUTE_REASON_TEXT = {
+    no_alarm: "hiện không có cảnh báo nào để tắt tiếng",
+    disabled: "thời gian tắt tiếng đang đặt là 0 phút (xem Cài đặt)",
+};
 const CFG_FIELDS = {
     temp_min: "s_tmin", temp_max: "s_tmax", ph_min: "s_pmin", ph_max: "s_pmax",
     turb_max_pct: "s_turb", level_min_pct: "s_lmin", level_danger_pct: "s_ldanger",
@@ -66,6 +72,8 @@
 let wentOffline = false;
 let pendingFeed = null;
 let feedTimer = null;
+let pendingMute = null;    // "mute" hoặc "unmute" khi đang chờ phản hồi
+let muteTimer = null;
 let history = [];
 let lastHistT = 0;
 let events = [];
@@ -336,11 +344,25 @@
     return Math.max(0, ...levels);
 }
 
+// Thanh tắt tiếng còi: chỉ hiện khi đang có cảnh báo. d.muted do firmware gửi trong telemetry.
+function renderMuteBar(risk, d) {
+    const bar = $("muteBar");
+    bar.hidden = risk < 1;
+    if (risk < 1) return;
+    const muted = d.muted === true;
+    $("muteBtn").hidden = muted;
+    $("unmuteBtn").hidden = !muted;
+    $("muteBtn").textContent = "🔕 Tắt tiếng còi " + activeCfg.mute_min + " phút";
+    if (muted && !pendingMute) showMsg("muteMsg", "Còi đang tắt tiếng. Tự bật lại khi cảnh báo tăng mức, hết cảnh báo hoặc hết giờ.", "");
+    else if (!muted && !pendingMute) showMsg("muteMsg", "", "");
+}
+
 function renderAlert(risk, d) {
     const al = $("alert"), rb = $("riskBadge");
     rb.textContent = risk === 0 ? "Cảnh báo: bình thường" : "Cảnh báo: mức " + risk;
     rb.className = "badge lv" + risk;
     document.title = risk >= 1 ? `⚠ Mức ${risk} · Bể cá thông minh` : "Bể cá thông minh";
+    renderMuteBar(risk, d);
     if (risk < 1) { al.hidden = true; return; }
     const names = flagListText(Array.isArray(d.flags) ? d.flags : []);
     const head = risk >= 3 ? "🚨 MỨC 3 · NGUY HIỂM" : risk === 2 ? "⚠ MỨC 2 · Cảnh báo cao" : "⚠ Mức 1 · Cảnh báo";
@@ -526,7 +548,7 @@
     let txt, cls, skip = false;
 
     if (type === "boot") { txt = "🔄 Thiết bị khởi động (firmware " + (e.fw || "?") + ")"; cls = "ev-boot"; }
-    else if (type === "feed_done") { txt = "🍤 Đã cho ăn " + (e.portions || 1) + " phần"; cls = "ev-feed"; }
+    else if (type === "feed_done") { txt = "🍤 Đã cho ăn " + (e.portions || 1) + " phần" + (e.id === "schedule" ? " (theo lịch)" : ""); cls = "ev-feed"; }
     else if (type === "feed_rejected") { txt = "⛔ Từ chối cho ăn: " + (REASON_TEXT[e.reason] || e.reason || "không rõ lý do"); cls = "ev-reject"; }
     else if (type === "alarm") {
         const lv = clamp(Math.round(Number(e.level)) || 1, 1, 3);
@@ -537,6 +559,10 @@
         if (last && (ts || now) - last.t < ALARM_GAP_MS[lv]) skip = true;
     }
 
+    else if (type === "mute") { txt = "🔕 Đã tắt tiếng còi " + (e.minutes || "?") + " phút"; cls = "ev-local"; }
+    else if (type === "unmute") { txt = "🔔 Đã bật lại tiếng còi"; cls = "ev-local"; }
+    else if (type === "mute_rejected") { txt = "⛔ Không tắt tiếng được: " + (MUTE_REASON_TEXT[e.reason] || e.reason || "không rõ lý do"); cls = "ev-reject"; }
+    else if (type === "config_rejected") { txt = "⛔ Thiết bị từ chối cài đặt: " + (e.reason || "không rõ lý do"); cls = "ev-reject"; }
     else { txt = "ℹ Sự kiện: " + type; cls = "ev-local"; }
 
 
@@ -545,6 +571,12 @@
     const k = ["ev", type, stamp, e.id || "", e.msg || "", e.level || "", e.reason || ""].join("|");
     if (!skip) pushEvent({ k, t: ts || now, cls, txt });
 
+    if (pendingMute) {
+        if (type === "mute" && pendingMute === "mute") endMute("✔ Đã tắt tiếng " + (e.minutes || "?") + " phút.", "ok");
+        else if (type === "unmute" && pendingMute === "unmute") endMute("✔ Đã bật lại tiếng còi.", "ok");
+        else if (type === "mute_rejected") endMute("✖ Không tắt tiếng được: " + (MUTE_REASON_TEXT[e.reason] || e.reason), "bad");
+    }
+
     if (pendingFeed && e.id === pendingFeed) {
         if (type === "feed_done") { endFeed(); showMsg("feedMsg", "✔ Đã cho ăn xong.", "ok"); }
         else if (type === "feed_rejected") { endFeed(); showMsg("feedMsg", "✖ Bị từ chối: " + (REASON_TEXT[e.reason] || e.reason), "bad"); }
@@ -641,6 +673,31 @@
     }, FEED_WAIT_MS);
 });
 
+/* ====== TẮT TIẾNG CÒI ====== */
+function endMute(text, cls) {
+    pendingMute = null;
+    clearTimeout(muteTimer);
+    $("muteBtn").disabled = false;
+    $("unmuteBtn").disabled = false;
+    showMsg("muteMsg", text, cls);
+}
+function sendMute(action) {
+    if (!client || !client.connected) { showMsg("muteMsg", "Chưa kết nối.", "bad"); return; }
+    if (action === "mute" && activeCfg.mute_min === 0) { showMsg("muteMsg", "Thời gian tắt tiếng đang đặt là 0 phút (xem Cài đặt).", "bad"); return; }
+    pendingMute = action;
+    $("muteBtn").disabled = true;
+    $("unmuteBtn").disabled = true;
+    showMsg("muteMsg", "Đang gửi lệnh…", "");
+    // KHÔNG đặt retain: lệnh retained sẽ bị giao lại mỗi lần thiết bị nối lại
+    client.publish(TOPIC.mute, JSON.stringify({ action }), { qos: 1, retain: false }, (err) => {
+        if (err) endMute("Gửi lệnh thất bại.", "bad");
+    });
+    clearTimeout(muteTimer);
+    muteTimer = setTimeout(() => { if (pendingMute === action) endMute("Chưa nhận được phản hồi từ thiết bị.", "warn"); }, MUTE_WAIT_MS);
+}
+$("muteBtn").addEventListener("click", () => sendMute("mute"));
+$("unmuteBtn").addEventListener("click", () => sendMute("unmute"));
+
 /* ====== CÀI ĐẶT ====== */
 function readNum(id) { const s = $(id).value.trim(); return s === "" ? NaN : Number(s); }
 function validateConfig(c) {
```

#### Tệp: `docs/index.html`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | ngay sau `<div id="alert">`: thêm thanh tắt tiếng | **THÊM** | `-46,6 +46,11` |

```diff
--- a/docs/index.html
+++ b/docs/index.html
@@ -46,6 +46,11 @@
     </header>
 
     <div id="alert" class="alert" hidden></div>
+    <div id="muteBar" class="mutebar" hidden>
+      <button id="muteBtn" type="button">🔕 Tắt tiếng còi</button>
+      <button id="unmuteBtn" type="button" class="ghost" hidden>🔔 Bật lại tiếng còi</button>
+      <span id="muteMsg" class="msg"></span>
+    </div>
 
     <section id="cards" class="grid">
       <div class="card nodata" id="c_temp">
```

#### Tệp: `docs/style.css`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | sau `.alert.old`: thêm kiểu `.mutebar` | **THÊM** | `-258,6 +258,15` |

```diff
--- a/docs/style.css
+++ b/docs/style.css
@@ -258,6 +258,15 @@
     animation: none;
 }
 
+/* ===== THANH TẮT TIẾNG CÒI ===== */
+.mutebar {
+    display: flex;
+    flex-wrap: wrap;
+    align-items: center;
+    gap: 8px;
+    margin: 0 0 10px;
+}
+
 /* ===== THẺ CHỈ SỐ ===== */
 .grid {
     display: grid;
```

#### Tệp: `docs/sw.js`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | hằng `CACHE`: tăng phiên bản | **SỬA** | `-1,7 +1,7` |

```diff
--- a/docs/sw.js
+++ b/docs/sw.js
@@ -1,7 +1,7 @@
 // sw.js: service worker tối thiểu cho PWA.
 // Chiến lược: ưu tiên mạng, mất mạng thì dùng bản đã lưu (tránh kẹt bản cũ khi nhóm cập nhật code).
 // Chỉ xử lý tệp cùng nguồn; CDN và WebSocket MQTT không bị can thiệp.
-const CACHE = "fishtank-shell-v2";
+const CACHE = "fishtank-shell-v3";
 const SHELL = ["./", "index.html", "style.css", "app.js", "manifest.json", "icons/icon.svg"];
 
 self.addEventListener("install", (e) => {
```

### Bước 3.4 ESP32 giả và mẫu MQTTX

ESP32 giả mô phỏng đúng logic tắt tiếng của firmware để thử web mà không cần thiết bị.

#### Tệp: `tools/fake-esp32/fake_esp32.py`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | `__init__`: thêm `t_mute` | **THÊM** | `-255,6 +255,7` |
| 2 | `__init__`: thêm trạng thái tắt tiếng | **THÊM** | `-271,6 +272,10` |
| 3 | `publish_telemetry()`: thêm trường `muted` | **SỬA** | `-352,7 +357,7` |
| 4 | `handle_message()`: nhánh `cmd/mute` | **THÊM** | `-370,6 +375,8` |
| 5 | thêm phương thức `handle_mute()` trước `reject()` | **THÊM** | `-382,6 +389,34` |
| 6 | `print_status()` | **SỬA** | `-471,7 +506,7` |
| 7 | `tick()`: tắt tiếng tự hủy và cờ `muted_changed` | **SỬA** | `-489,7 +524,13` |

```diff
--- a/tools/fake-esp32/fake_esp32.py
+++ b/tools/fake-esp32/fake_esp32.py
@@ -255,6 +255,7 @@
         self.t_event = f"fishtank/{self.tank}/event"
         self.t_cmd = f"fishtank/{self.tank}/cmd/#"
         self.t_feed = f"fishtank/{self.tank}/cmd/feed"
+        self.t_mute = f"fishtank/{self.tank}/cmd/mute"
         self.t_config = f"fishtank/{self.tank}/config"
 
         self.q = queue.Queue()
@@ -271,6 +272,10 @@
         self.next_sample = 0.0
         self.last_publish = 0.0
 
+        self.muted = False             # tắt tiếng còi (cmd/mute)
+        self.mute_until = 0.0          # monotonic
+        self.mute_risk = 0             # risk lúc bấm tắt tiếng
+
         self.last_feed_id = ""
         self.feeding = None            # {"id","portions","end"}
         self.last_feed_done = None     # monotonic
@@ -352,7 +357,7 @@
             "v": 1, "ts": int(time.time()),
             "temp_c": r["temp_c"], "ph": r["ph"], "turb_v": r["turb_v"],
             "turb_pct": r["turb_pct"], "level_pct": r["level_pct"],
-            "risk": self.risk, "flags": self.flags,
+            "risk": self.risk, "flags": self.flags, "muted": self.muted,
             "rssi": int(round(random.gauss(-61, 2))),
             "uptime_s": int(time.monotonic() - self.t0),
             "fw": FW_VERSION,
@@ -370,6 +375,8 @@
             return
         if topic == self.t_feed:
             self.handle_feed(doc)
+        elif topic == self.t_mute:
+            self.handle_mute(doc)
         elif topic == self.t_config:
             self.handle_config(doc)
 
@@ -382,6 +389,34 @@
         self.force_publish = True
         log(f"[config] đã áp dụng: {json.dumps(new, ensure_ascii=False)}")
 
+    def handle_mute(self, doc):
+        """cmd/mute: {"action":"mute"} hoặc {"action":"unmute"} (giống firmware: Alarm::mute / unmute)."""
+        if not isinstance(doc, dict):
+            return
+        action = doc.get("action")
+        if action == "unmute":
+            self.muted = False
+            self.force_publish = True
+            log("[mute] đã bỏ tắt tiếng")
+            self.publish_event(type="unmute", ts=int(time.time()))
+        elif action == "mute":
+            minutes = int(self.cfg["mute_min"])
+            if minutes == 0:
+                log("[mute] từ chối: mute_min = 0 (disabled)")
+                self.publish_event(type="mute_rejected", reason="disabled", ts=int(time.time()))
+            elif self.risk == 0:
+                log("[mute] từ chối: không có cảnh báo (no_alarm)")
+                self.publish_event(type="mute_rejected", reason="no_alarm", ts=int(time.time()))
+            else:
+                self.muted = True
+                self.mute_risk = self.risk
+                self.mute_until = time.monotonic() + minutes * 60.0
+                self.force_publish = True
+                log(f"[mute] tắt tiếng {minutes} phút (risk lúc bấm = {self.risk})")
+                self.publish_event(type="mute", minutes=minutes, ts=int(time.time()))
+        else:
+            log(f"[mute] bỏ qua action lạ: {action!r}")
+
     def reject(self, fid, reason):
         log(f"[cho ăn] từ chối id={fid}: {reason}")
         self.publish_event(type="feed_rejected", id=fid, reason=reason)
@@ -471,7 +506,7 @@
         r = self.reading or {}
         feeding = f"đang cho ăn id={self.feeding['id']}" if self.feeding else "rảnh"
         log(f"kết nối={'có' if self.connected else 'KHÔNG'} | risk={self.risk} | "
-            f"mức từng cờ={self.levels} | giá trị={r} | cho ăn: {feeding}, "
+            f"mức từng cờ={self.levels} | tắt tiếng={'có' if self.muted else 'không'} | giá trị={r} | cho ăn: {feeding}, "
             f"{len(self.feeds)} lần trong 24h (tối đa {self.args.max_per_day}, "
             f"cách nhau {self.args.min_interval:g}s)")
         log(f"config hiện tại: {json.dumps(self.cfg, ensure_ascii=False)}")
@@ -489,7 +524,13 @@
         if self.force_publish or now >= self.next_sample:
             self.reading = self.sample()
             risk, flags, levels = evaluate(self.reading, self.cfg)
-            changed = (risk != self.risk) or (flags != self.flags)
+            muted_changed = False
+            # Giống Alarm::tick: tắt tiếng tự hủy khi risk tăng quá mức lúc bấm, khi risk về 0, hoặc hết giờ
+            if self.muted and (risk == 0 or risk > self.mute_risk or now >= self.mute_until):
+                self.muted = False
+                muted_changed = True
+                log("[mute] tắt tiếng tự hủy (risk tăng, về 0 hoặc hết giờ)")
+            changed = (risk != self.risk) or (flags != self.flags) or muted_changed
             if risk > self.risk and self.connected:
                 self.publish_event(type="alarm", level=risk, msg=",".join(flags), ts=int(time.time()))
             self.risk, self.flags, self.levels = risk, flags, levels
```

#### Tệp: `tools/mqtt-samples/README.md`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | bảng mẫu: thêm dòng cho `cmd-*.json` | **THÊM** | `-7,6 +7,7` |

```diff
--- a/tools/mqtt-samples/README.md
+++ b/tools/mqtt-samples/README.md
@@ -7,6 +7,7 @@
 | `telemetry-*.json` | fishtank/tank01/telemetry | 0 | bật |
 | `event-*.json` | fishtank/tank01/event | 0 | tắt |
 | (chữ) `online` / `offline` | fishtank/tank01/status | 1 | bật |
+| `cmd-feed.json`, `cmd-mute.json`, `cmd-unmute.json` | fishtank/tank01/cmd/feed, cmd/mute (kết nối bằng tài khoản `webuser`) | 1 | **tắt** (lệnh retained sẽ bị giao lại mỗi lần ESP32 nối lại) |
 
 Last Will cấu hình trong MQTTX: topic status, QoS 1, Retain bật, payload `offline`.
 Với `event-feed-*`, thay `id` bằng đúng `id` mà web vừa gửi trong `cmd/feed`.
```

#### Tệp: `tools/mqtt-samples/cmd-mute.json`

**THÊM (tệp mới)**: mẫu lệnh tắt tiếng (publish bằng tài khoản `webuser`, QoS 1, KHÔNG retain).

```diff
--- /dev/null
+++ b/tools/mqtt-samples/cmd-mute.json
@@ -0,0 +1 @@
+{"action": "mute"}
```

#### Tệp: `tools/mqtt-samples/cmd-unmute.json`

**THÊM (tệp mới)**: mẫu lệnh bật lại tiếng.

```diff
--- /dev/null
+++ b/tools/mqtt-samples/cmd-unmute.json
@@ -0,0 +1 @@
+{"action": "unmute"}
```

#### Tệp: `tools/mqtt-samples/event-mute.json`

**THÊM (tệp mới)**: mẫu event ESP32 báo đã tắt tiếng.

```diff
--- /dev/null
+++ b/tools/mqtt-samples/event-mute.json
@@ -0,0 +1 @@
+{"type": "mute", "minutes": 30, "ts": 1793236700}
```

#### Tệp: `tools/mqtt-samples/event-unmute.json`

**THÊM (tệp mới)**: mẫu event ESP32 báo đã bật lại tiếng.

```diff
--- /dev/null
+++ b/tools/mqtt-samples/event-unmute.json
@@ -0,0 +1 @@
+{"type": "unmute", "ts": 1793236760}
```

#### Tệp: `tools/mqtt-samples/event-mute-rejected.json`

**THÊM (tệp mới)**: mẫu event từ chối tắt tiếng (`no_alarm`).

```diff
--- /dev/null
+++ b/tools/mqtt-samples/event-mute-rejected.json
@@ -0,0 +1 @@
+{"type": "mute_rejected", "reason": "no_alarm", "ts": 1793236800}
```

#### Tệp: `tools/mqtt-samples/telemetry-normal.json`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | dòng JSON duy nhất: thêm `"muted": false` sau `"risk"` | **SỬA** | `-1 +1` |

```diff
--- a/tools/mqtt-samples/telemetry-normal.json
+++ b/tools/mqtt-samples/telemetry-normal.json
@@ -1 +1 @@
-{"v": 1, "ts": 1793236574, "temp_c": 27.5, "ph": 7.12, "turb_v": 2.85, "turb_pct": 12, "level_pct": 80, "risk": 0, "flags": [], "rssi": -61, "uptime_s": 3600, "fw": "1.0.0"}
+{"v": 1, "ts": 1793236574, "temp_c": 27.5, "ph": 7.12, "turb_v": 2.85, "turb_pct": 12, "level_pct": 80, "risk": 0, "muted": false, "flags": [], "rssi": -61, "uptime_s": 3600, "fw": "1.0.0"}
```

#### Tệp: `tools/mqtt-samples/telemetry-warning.json`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | dòng JSON duy nhất: thêm `"muted": false` sau `"risk"` | **SỬA** | `-1 +1` |

```diff
--- a/tools/mqtt-samples/telemetry-warning.json
+++ b/tools/mqtt-samples/telemetry-warning.json
@@ -1 +1 @@
-{"v": 1, "ts": 1793236584, "temp_c": 31.2, "ph": 7.0, "turb_v": 2.9, "turb_pct": 15, "level_pct": 78, "risk": 1, "flags": ["temp_high"], "rssi": -63, "uptime_s": 3610, "fw": "1.0.0"}
+{"v": 1, "ts": 1793236584, "temp_c": 31.2, "ph": 7.0, "turb_v": 2.9, "turb_pct": 15, "level_pct": 78, "risk": 1, "muted": false, "flags": ["temp_high"], "rssi": -63, "uptime_s": 3610, "fw": "1.0.0"}
```

#### Tệp: `tools/mqtt-samples/telemetry-danger.json`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | dòng JSON duy nhất: thêm `"muted": false` sau `"risk"` | **SỬA** | `-1 +1` |

```diff
--- a/tools/mqtt-samples/telemetry-danger.json
+++ b/tools/mqtt-samples/telemetry-danger.json
@@ -1 +1 @@
-{"v": 1, "ts": 1793236594, "temp_c": 27.5, "ph": 5.8, "turb_v": 3.4, "turb_pct": 55, "level_pct": 12, "risk": 2, "flags": ["ph_low", "turb_high", "level_low"], "rssi": -70, "uptime_s": 3620, "fw": "1.0.0"}
+{"v": 1, "ts": 1793236594, "temp_c": 27.5, "ph": 5.8, "turb_v": 3.4, "turb_pct": 55, "level_pct": 12, "risk": 2, "muted": false, "flags": ["ph_low", "turb_high", "level_low"], "rssi": -70, "uptime_s": 3620, "fw": "1.0.0"}
```

#### Tệp: `tools/mqtt-samples/telemetry-sensor-error.json`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | dòng JSON duy nhất: thêm `"muted": false` sau `"risk"` | **SỬA** | `-1 +1` |

```diff
--- a/tools/mqtt-samples/telemetry-sensor-error.json
+++ b/tools/mqtt-samples/telemetry-sensor-error.json
@@ -1 +1 @@
-{"v": 1, "ts": 1793236604, "temp_c": null, "ph": 7.1, "turb_v": 2.85, "turb_pct": 12, "level_pct": 80, "risk": 1, "flags": ["sensor_temp"], "rssi": -61, "uptime_s": 3630, "fw": "1.0.0"}
+{"v": 1, "ts": 1793236604, "temp_c": null, "ph": 7.1, "turb_v": 2.85, "turb_pct": 12, "level_pct": 80, "risk": 1, "muted": false, "flags": ["sensor_temp"], "rssi": -61, "uptime_s": 3630, "fw": "1.0.0"}
```

### Bước 3.5 Bộ ca thử còi

Thay ca "mức 3 kêu liên tục" bằng ca mẫu 3 tiếng và thêm ca tắt tiếng ở mức 3.

#### Tệp: `tools/test-sensors-alarm/test_sensors_alarm.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | khối alarm mức 3: thay ca "kêu liên tục" bằng ca 3 tiếng mỗi 5 giây | **SỬA** | `-228,7 +228,13` |
| 2 | phần `alarm: tắt tiếng`: thêm ca tắt tiếng ở mức 3 | **THÊM** | `-268,6 +274,12` |

```diff
--- a/tools/test-sensors-alarm/test_sensors_alarm.cpp
+++ b/tools/test-sensors-alarm/test_sensors_alarm.cpp
@@ -228,7 +228,13 @@
     Alarm a;
     AlarmOut o = tk(a, 1000, 3);
     expect(o.on && o.dutyPct == BUZZ_DUTY_LOUD_PCT, "Mức 3: kêu to nhất");
-    expect(tk(a, 1500, 3).on && tk(a, 31000, 3).on && tk(a, 999999, 3).on, "Mức 3: kêu liên tục");
+    expect(tk(a, 1199, 3).on, "Mức 3: tiếng thứ nhất kéo dài 200 ms");
+    expect(!tk(a, 1200, 3).on && !tk(a, 1399, 3).on, "Mức 3: nghỉ 200 ms");
+    expect(tk(a, 1400, 3).on && tk(a, 1599, 3).on && tk(a, 1400, 3).dutyPct == BUZZ_DUTY_LOUD_PCT, "Mức 3: tiếng thứ hai 200 ms, vẫn to nhất");
+    expect(!tk(a, 1600, 3).on && !tk(a, 1799, 3).on, "Mức 3: nghỉ 200 ms");
+    expect(tk(a, 1800, 3).on && tk(a, 1999, 3).on, "Mức 3: tiếng thứ ba 200 ms");
+    expect(!tk(a, 2000, 3).on && !tk(a, 5999, 3).on, "Mức 3: im lặng đến hết chu kỳ 5 giây");
+    expect(tk(a, 6000, 3).on && tk(a, 6400, 3).on && tk(a, 6800, 3).on, "Mức 3: lặp lại đúng 5 giây sau (3 tiếng nữa)");
     expect(!tk(a, 1000000, 0).on, "Risk về 0: im ngay");
   }
   {
@@ -268,6 +274,12 @@
   }
   {
     Alarm a; a.configure(true, 30);
+    tk(a, 0, 3); a.mute(0);
+    expect(a.muted() && !tk(a, 5000, 3).on && !tk(a, 1799999, 3).on, "Mức 3: tắt tiếng đủ 30 phút, kể cả đúng lúc vốn có tiếng");
+    expect(tk(a, 1800000, 3).on && !a.muted(), "Mức 3: hết 30 phút, còi kêu lại theo mẫu 3 tiếng");
+  }
+  {
+    Alarm a; a.configure(true, 30);
     tk(a, 0, 1); a.mute(10);
     expect(!tk(a, 20, 1).on, "Đang tắt tiếng ở mức 1");
     expect(tk(a, 5000, 2).on && !a.muted(), "Risk tăng: tắt tiếng bị hủy, kêu ngay");
```

**Kiểm tra sau Phase 3:** `test_sensors_alarm` **101 ca, 0 sai**; `test_risk` 72; `test_store_feeder` 83; `node --check`; `py_compile`; kịch bản ESP32 giả ngoại tuyến (mục 4.3) in `ĐẠT`; kiểm tra cú pháp bằng tệp giả (mục 4.4) đạt. Phần thử bằng broker thật và tiếng còi thật là việc của nhóm (mục 5).

**Commit gợi ý:** `buzzer: muc 3 la 3 tieng moi 5 giay; them cmd/mute, nut tat tieng tren web, event mute/unmute/mute_rejected`


---

## Phase 4: Độ bền và NVS (#6, #7)

**Điều kiện:** kiểm tra sau Phase 3 đạt.

### Bước 4.1 Khởi động lại thông minh khi mất mạng (#6)

Còn WiFi mà mất MQTT: khởi động lại sau 10 phút như cũ. Mất cả WiFi: chờ 30 phút (`REBOOT_IF_NO_WIFI_MS`). Không bao giờ khởi động lại khi đang có cảnh báo (`risk > 0`) hoặc servo đang quay, để không ngắt còi và không mất bộ lọc risk.

#### Tệp: `firmware/fish_tank/config.h`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | `REBOOT_IF_NO_MQTT_MS`: sửa chú thích và thêm `REBOOT_IF_NO_WIFI_MS` | **SỬA** | `-162,5 +162,6` |

```diff
--- a/firmware/fish_tank/config.h
+++ b/firmware/fish_tank/config.h
@@ -162,5 +162,6 @@
 #define TZ_OFFSET_S             (7 * 3600) // UTC+7
 #define RECONNECT_MIN_MS        1000       // chờ tăng dần 1, 2, 4 ... giây
 #define RECONNECT_MAX_MS        30000      // tối đa 30 giây
-#define REBOOT_IF_NO_MQTT_MS    (10UL * 60UL * 1000UL)  // mất MQTT quá 10 phút thì tự khởi động lại
+#define REBOOT_IF_NO_MQTT_MS    (10UL * 60UL * 1000UL)  // WiFi còn mà mất MQTT quá 10 phút thì tự khởi động lại
+#define REBOOT_IF_NO_WIFI_MS    (30UL * 60UL * 1000UL)  // mất cả WiFi: chờ lâu hơn (30 phút) mới khởi động lại, tránh làm gián đoạn còi
 #define WDT_TIMEOUT_S           30
```

#### Tệp: `firmware/fish_tank/fish_tank.ino`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | bước 5a: nhận config mới (so sánh trước khi ghi NVS) | **SỬA** | `-233,10 +233,14` |
| 2 | bước 8: điều kiện tự khởi động lại | **SỬA** | `-259,8 +263,11` |

```diff
--- a/firmware/fish_tank/fish_tank.ino
+++ b/firmware/fish_tank/fish_tank.ino
@@ -233,10 +233,14 @@
   // 5a) Cấu hình mới từ web (đã gộp và đã validate trong net)
   AppConfig newCfg;
   if (netTakeConfig(newCfg)) {
-    g_cfg = newCfg;
-    if (!storeSaveConfig(g_cfg)) Serial.println("[store] LOI: khong luu duoc cau hinh vao NVS");
-    g_alarm.configure(g_cfg.buzzer_on, g_cfg.mute_min);
-    Serial.println("[config] da ap dung cau hinh moi");
+    if (configEqual(g_cfg, newCfg)) {
+      Serial.println("[config] giong cau hinh dang dung, bo qua (khong ghi NVS)");   // config retained gui lai moi lan noi lai
+    } else {
+      g_cfg = newCfg;
+      if (!storeSaveConfig(g_cfg)) Serial.println("[store] LOI: khong luu duoc cau hinh vao NVS");
+      g_alarm.configure(g_cfg.buzzer_on, g_cfg.mute_min);
+      Serial.println("[config] da ap dung cau hinh moi");
+    }
   }
 
   // 5b) Lệnh cho ăn từ web
@@ -259,8 +263,11 @@
     logStatus();
   }
 
-  // 8) Mất MQTT quá REBOOT_IF_NO_MQTT_MS thì tự khởi động lại (không làm khi servo đang quay)
-  if (!g_runner.busy() && netMqttDownMs(now) >= REBOOT_IF_NO_MQTT_MS) {
+  // 8) Mất MQTT quá lâu thì tự khởi động lại (không làm khi servo đang quay, không làm khi đang có cảnh báo).
+  //    WiFi còn mà MQTT hỏng: sau REBOOT_IF_NO_MQTT_MS (10 phút). Mất cả WiFi: sau REBOOT_IF_NO_WIFI_MS (30 phút),
+  //    vì khởi động lại không sửa được WiFi nhà mà còn làm mất bộ lọc risk và ngắt còi.
+  uint32_t rebootAfterMs = netWifiUp() ? REBOOT_IF_NO_MQTT_MS : REBOOT_IF_NO_WIFI_MS;
+  if (!g_runner.busy() && g_risk.risk == 0 && netMqttDownMs(now) >= rebootAfterMs) {
     Serial.println("[wdt] mat MQTT qua lau, khoi dong lai");
     g_runner.reset();
     feederApply(g_runner);               // chắc chắn servo dừng
```

### Bước 4.2 Không ghi NVS khi cấu hình không đổi (#7)

Config retained được broker giao lại mỗi lần nối lại MQTT. So sánh từng trường (`configEqual`) rồi mới ghi, tránh mòn flash khi WiFi chập chờn. Phần tương ứng trong `fish_tank.ino` (bước 5a) là hunk số 1 của khối diff `fish_tank.ino` ở bước 4.1 (cùng một tệp, áp một lần).

#### Tệp: `firmware/fish_tank/store.h`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | sau khai báo `configThresholds()`: thêm khai báo `configEqual()` | **THÊM** | `-24,6 +24,9` |

```diff
--- a/firmware/fish_tank/store.h
+++ b/firmware/fish_tank/store.h
@@ -24,6 +24,9 @@
 AppConfig      configDefaults();
 RiskThresholds configThresholds(const AppConfig& c);
 
+// So sánh từng trường (không dùng memcmp vì struct có khoảng đệm). feed_min chỉ so sánh feed_count phần tử đầu.
+bool configEqual(const AppConfig& a, const AppConfig& b);
+
 // Kiểm tra TOÀN BỘ cấu hình (đã gộp giá trị mới vào giá trị cũ). Sai bất kỳ điều nào thì trả false và
 // ghi lý do vào err; khi đó phải TỪ CHỐI cả bản tin config (giống validate_config của fake_esp32.py).
 bool configValidate(const AppConfig& c, char* err, size_t errLen);
```

#### Tệp: `firmware/fish_tank/store.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | thêm hàm `configEqual()` trước `configThresholds()` | **THÊM** | `-23,6 +23,19` |

```diff
--- a/firmware/fish_tank/store.cpp
+++ b/firmware/fish_tank/store.cpp
@@ -23,6 +23,19 @@
   return c;
 }
 
+bool configEqual(const AppConfig& a, const AppConfig& b) {
+  if (a.temp_min != b.temp_min || a.temp_max != b.temp_max) return false;
+  if (a.ph_min != b.ph_min || a.ph_max != b.ph_max) return false;
+  if (a.turb_max_pct != b.turb_max_pct) return false;
+  if (a.level_min_pct != b.level_min_pct || a.level_danger_pct != b.level_danger_pct) return false;
+  if (a.buzzer_on != b.buzzer_on || a.mute_min != b.mute_min || a.feed_ms != b.feed_ms) return false;
+  if (a.feed_count != b.feed_count) return false;
+  for (uint8_t i = 0; i < a.feed_count && i < FEED_MAX_TIMES; i++) {
+    if (a.feed_min[i] != b.feed_min[i]) return false;
+  }
+  return true;
+}
+
 RiskThresholds configThresholds(const AppConfig& c) {
   RiskThresholds t;
   t.temp_min = c.temp_min;
```

### Bước 4.3 Bộ ca thử

Thêm ca cho `configEqual()`.

#### Tệp: `tools/test-store-feeder/test_store_feeder.cpp`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | đầu `main()`: thêm các ca `configEqual()` | **THÊM** | `-44,6 +44,18` |

```diff
--- a/tools/test-store-feeder/test_store_feeder.cpp
+++ b/tools/test-store-feeder/test_store_feeder.cpp
@@ -44,6 +44,18 @@
   printf("=== store: kiểm tra cấu hình ===\n");
   AppConfig c = configDefaults();
   expect(configValidate(c, err, sizeof(err)), "Cấu hình mặc định hợp lệ");
+  { AppConfig a = configDefaults(), b = configDefaults();
+    expect(configEqual(a, b), "configEqual: hai cấu hình mặc định giống nhau");
+    b = a; b.temp_max = 29.5f;
+    expect(!configEqual(a, b), "configEqual: đổi nhiệt độ tối đa thì khác");
+    b = a; b.feed_min[1] = 15 * 60;
+    expect(!configEqual(a, b), "configEqual: đổi một mốc giờ cho ăn thì khác");
+    b = a; b.feed_count = 2;
+    expect(!configEqual(a, b), "configEqual: đổi số mốc cho ăn thì khác");
+    a.feed_count = 2; b = a; b.feed_min[2] = 7 * 60;
+    expect(configEqual(a, b), "configEqual: mốc nằm ngoài feed_count không được so sánh");
+    b = a; b.buzzer_on = !a.buzzer_on;
+    expect(!configEqual(a, b), "configEqual: đổi buzzer_on thì khác"); }
   expect(c.feed_count == 3 && c.feed_min[0] == 480 && c.feed_min[1] == 840 && c.feed_min[2] == 1080,
          "Lịch cho ăn mặc định: 3 mốc 08:00, 14:00, 18:00");
   { RiskThresholds th = configThresholds(c);
```

**Kiểm tra sau Phase 4:** `test_store_feeder` **89 ca, 0 sai**; mục 4.4 đạt; `grep -n "REBOOT_IF_NO" firmware/fish_tank/*.ino firmware/fish_tank/config.h` chỉ thấy hai hằng `REBOOT_IF_NO_MQTT_MS` và `REBOOT_IF_NO_WIFI_MS` được dùng. Hành vi khởi động lại thật chỉ thử được trên ESP32 (T3.10, T5.5, T5.7), nhóm làm.

**Commit gợi ý:** `resilience: khong reboot khi dang canh bao hoac mat WiFi qua ngan; khong ghi NVS khi config khong doi`


---

## Phase 5: Kiểm thử và xác nhận

**Điều kiện:** kiểm tra sau Phase 4 đạt.

### Bước 5.1 Đồng bộ README gốc

Cập nhật số ca thử của giai đoạn 3. Không sửa `tests/phase*/results.md` (đó là bằng chứng của nhóm).

#### Tệp: `README.md`

| # | Vị trí tác động | Thao tác | Số dòng gốc (ngay trước Phase này) |
|---|---|---|---|
| 1 | dòng giai đoạn 3 (≈ dòng 77): cập nhật số ca thử | **SỬA** | `-74,7 +74,7` |

```diff
--- a/README.md
+++ b/README.md
@@ -74,7 +74,7 @@
 | 0 | Chuẩn bị môi trường | **Cơ bản hoàn tất.** Đã cài công cụ/thư viện, nạp chương trình thử, tạo kho và mời thành viên. Nhóm đã báo xử lý sự cố bí mật và đổi mật khẩu; vẫn cần chạy lại rà soát kho (T0.4). |
 | 1 | Dựng cloud EMQX | **Gần hoàn tất.** Đã tạo deployment/tài khoản, cấu hình quyền, thử MQTT/TLS và tải CA. Còn T1.9; bài kiểm tra bổ sung T1.8 về quyền tài khoản `esp32` đã được nhóm chủ động bỏ qua nên chưa có bằng chứng. |
 | 2 | Kiểm tra và hiệu chuẩn thiết bị | **Đang làm, bị giới hạn bởi phần cứng đang cho mượn.** DS18B20 đã đọc được; servo đã quay; còi đã kêu. Còn kiểm tra mực nước, pH, độ đục, cơ cấu cho ăn, so nhiệt kế và hoàn thiện bảng hiệu chuẩn. |
-| 3 | Firmware ESP32 | **Đã viết code; kiểm thử logic MÔ PHỎNG đạt.** Risk: 35/35 ca; logic cảm biến/còi: 83 ca, 0 lỗi; logic cho ăn/lưu cấu hình: 76 ca, 0 lỗi. Chưa có bằng chứng biên dịch, nạp và chạy trên ESP32 thật; hằng số hiệu chuẩn còn tạm. |
+| 3 | Firmware ESP32 | **Đã viết code; kiểm thử logic MÔ PHỎNG đạt.** Risk: 72/72 ca; logic cảm biến/còi: 101 ca, 0 lỗi; logic cho ăn/lưu cấu hình: 89 ca, 0 lỗi (bộ ca thử bản 1.4, chạy lại 01/11/2026; ảnh bằng chứng cũ ở tests/phase3/ ứng với bộ 35/83/76 ca). Chưa có bằng chứng biên dịch, nạp và chạy trên ESP32 thật; hằng số hiệu chuẩn còn tạm. |
 | 4 | Web frontend | **Đã viết code; đã thử với ESP32 giả (MÔ PHỎNG).** Có ảnh cho một phần bài thử, nhưng chưa đủ bằng chứng để kết luận toàn bộ T4.1–T4.9 đạt. Chưa thử với ESP32 thật và GitHub Pages chưa bật. |
 | 5 | Ghép nối, thử nghiệm toàn hệ thống | **Chưa làm.** Cần thực hiện các kịch bản T5.1–T5.10, gồm cảm biến, cho ăn, cảnh báo, mất mạng, chạy liên tục và truy cập từ mạng di động. |
 | 6 | Đưa web lên mạng, bảo mật | **Chưa làm.** Cần bật GitHub Pages, chuyển firmware sang xác minh TLS bằng CA, kiểm tra ACL, rà soát bí mật và hoàn tất các bài T6.1–T6.5. |
```

**Kiểm tra sau Phase 5:** chạy toàn bộ mục 4 một lượt (kết quả cuối: **72, 101, 89 ca, 0 sai**), rồi `git status` chỉ chứa các tệp ở mục 2 (không có `secrets.h`, `*.exe`, `fix-plan-patches/`, `docs/icons/icon.svg`).

**Commit gợi ý:** `docs: dong bo README (so ca thu) va chot kiem thu ban 1.4`


---

## 4. Kịch bản kiểm thử và xác nhận (Verification Script)

Chạy từ **thư mục gốc của kho** (nơi có thư mục `.git`). Trên Windows dùng PowerShell; lệnh `g++` giống phần đầu các tệp test (tệp `.exe` đã nằm trong `.gitignore`). Nếu ô đầu ra không thấy `TẤT CẢ ĐẠT` hoặc số ca sai khác 0: **dừng, đừng sửa bộ ca thử cho khớp code**; báo nhóm kèm các dòng `FAIL`.

### 4.1 Ba bộ ca thử logic trên máy tính

```powershell
# Windows PowerShell
chcp 65001
g++ -std=c++11 -Wall -I firmware/fish_tank tools/test-risk/test_risk.cpp firmware/fish_tank/risk.cpp -o tools/test-risk/test_risk.exe
.\tools\test-risk\test_risk.exe
g++ -std=c++11 -Wall -I firmware/fish_tank tools/test-sensors-alarm/test_sensors_alarm.cpp firmware/fish_tank/sensors.cpp firmware/fish_tank/alarm.cpp firmware/fish_tank/risk.cpp -o tools/test-sensors-alarm/test_sensors_alarm.exe
.\tools\test-sensors-alarm\test_sensors_alarm.exe
g++ -std=c++11 -Wall -I firmware/fish_tank tools/test-store-feeder/test_store_feeder.cpp firmware/fish_tank/store.cpp firmware/fish_tank/feeder.cpp firmware/fish_tank/risk.cpp -o tools/test-store-feeder/test_store_feeder.exe
.\tools\test-store-feeder\test_store_feeder.exe
```

```bash
# Linux, macOS, Git Bash
g++ -std=c++11 -Wall -I firmware/fish_tank tools/test-risk/test_risk.cpp firmware/fish_tank/risk.cpp -o /tmp/test_risk && /tmp/test_risk | tail -3
g++ -std=c++11 -Wall -I firmware/fish_tank tools/test-sensors-alarm/test_sensors_alarm.cpp firmware/fish_tank/sensors.cpp firmware/fish_tank/alarm.cpp firmware/fish_tank/risk.cpp -o /tmp/test_sa && /tmp/test_sa | tail -3
g++ -std=c++11 -Wall -I firmware/fish_tank tools/test-store-feeder/test_store_feeder.cpp firmware/fish_tank/store.cpp firmware/fish_tank/feeder.cpp firmware/fish_tank/risk.cpp -o /tmp/test_sf && /tmp/test_sf | tail -3
```

**Kết quả mong đợi theo từng thời điểm** (mỗi dòng cuối là `Tổng: N ca, sai 0` rồi `TẤT CẢ ĐẠT`; không có cảnh báo biên dịch nào với `-Wall`):

| Sau | test_risk | test_sensors_alarm | test_store_feeder |
|---|---|---|---|
| Trước khi sửa (gốc) | 35 | 83 | 76 |
| Phase 1 | 72 | 93 | 77 |
| Phase 2 | 72 | 93 | 83 |
| Phase 3 | 72 | 101 | 83 |
| Phase 4 và 5 | **72** | **101** | **89** |

### 4.2 Web và ESP32 giả: kiểm tra cú pháp

```bash
node --check docs/app.js
python -m py_compile tools/fake-esp32/fake_esp32.py     # trên Linux: python3
```

Cả hai không in gì là đạt. (Xóa thư mục `tools/fake-esp32/__pycache__` nếu `py_compile` tạo ra; không commit.)

### 4.3 Kịch bản ESP32 giả ngoại tuyến (không cần broker, không cần mật khẩu)

Kiểm tra logic `cmd/mute`, lỗi cảm biến mới và lịch mặc định của `fake_esp32.py` bằng một MQTT client giả. Lưu thành `fake_offline_test.py` ở thư mục gốc kho (không commit) rồi chạy `python fake_offline_test.py`. Kết quả đạt: dòng cuối `ĐẠT | đạt 15 | sai 0`, mã thoát 0.

```python
import sys, types, json
# paho-mqtt có thể chưa cài: tạo bản giả đủ để import fake_esp32
try:
    import paho.mqtt.client
except Exception:
    paho = types.ModuleType("paho"); m = types.ModuleType("paho.mqtt"); c = types.ModuleType("paho.mqtt.client")
    c.Client = object; paho.mqtt = m; m.client = c
    sys.modules.update({"paho": paho, "paho.mqtt": m, "paho.mqtt.client": c})
sys.path.insert(0, "tools/fake-esp32")
import fake_esp32 as F
from argparse import Namespace

class Dummy:                                   # thay cho MQTT client thật
    def __init__(s): s.sent = []
    def publish(s, topic, payload, qos=0, retain=False):
        s.sent.append((topic, json.loads(payload) if payload not in ("online", "offline") else payload))

F.FakeEsp32._make_client = lambda self, pw: Dummy()
args = Namespace(tank="tank01", min_interval=60.0, max_per_day=4, omit_null=False, interval=10.0,
                 host="x", port=1, user="u", ca=None, debug=False)
e = F.FakeEsp32(args, "khong-dung-mat-khau-that"); e.connected = True
ok = bad = 0
def chk(cond, name):
    global ok, bad
    if cond: ok += 1
    else: bad += 1; print("SAI:", name)
def events(): return [d for t, d in e.client.sent if t.endswith("/event")]
def tele():   return [d for t, d in e.client.sent if t.endswith("/telemetry")][-1]

chk(e.cfg["feed_times"] == ["08:00", "14:00", "18:00"], "mặc định 3 mốc cho ăn")
e.handle_message(e.t_mute, b'{"action":"mute"}')
chk(events()[-1]["reason"] == "no_alarm", "mute khi không có cảnh báo: từ chối no_alarm")
e.handle_command("hot"); e.tick(); chk(e.risk == 1, "hot -> risk 1")
e.handle_message(e.t_mute, b'{"action":"mute"}')
chk(e.muted and events()[-1]["type"] == "mute" and events()[-1]["minutes"] == 30, "mute ở risk 1")
e.tick(); chk(tele()["muted"] is True, "telemetry có muted = true")
e.handle_command("set temp 32"); e.tick(); chk(e.risk == 2 and not e.muted, "risk tăng 1 -> 2: tắt tiếng bị hủy")
e.handle_message(e.t_mute, b'{"action":"mute"}'); chk(e.muted, "mute ở risk 2")
e.handle_command("hot"); e.tick(); chk(e.risk == 1 and e.muted, "risk giảm 2 -> 1: vẫn tắt tiếng")
e.handle_command("normal"); e.tick(); chk(e.risk == 0 and not e.muted, "risk về 0: hủy tắt tiếng")
e.handle_command("hot"); e.tick(); e.handle_message(e.t_mute, b'{"action":"unmute"}')
chk(not e.muted and events()[-1]["type"] == "unmute", "unmute")
e.cfg["mute_min"] = 0; e.handle_message(e.t_mute, b'{"action":"mute"}')
chk(events()[-1]["reason"] == "disabled", "mute_min = 0: từ chối disabled")
e.handle_command("normal"); e.handle_command("fail turb"); e.tick()
chk(e.flags == ["sensor_turb"] and e.risk == 1 and tele()["turb_pct"] is None and tele()["turb_v"] is None, "fail turb -> sensor_turb, giá trị null")
e.handle_command("fail level"); e.tick()
chk(sorted(e.flags) == ["sensor_level", "sensor_turb"] and tele()["level_pct"] is None, "fail level -> sensor_level, giá trị null")
e.handle_command("fail temp"); e.handle_command("fail ph"); e.tick()
chk(e.risk == 3 and len(e.flags) == 4, "4 cảm biến lỗi -> risk 3")
e.handle_command("fix"); e.tick(); chk(e.risk == 0 and e.flags == [], "fix -> về bình thường")
print("ĐẠT" if bad == 0 else "CÓ LỖI", "| đạt", ok, "| sai", bad)
sys.exit(0 if bad == 0 else 1)
```

### 4.4 Kiểm tra cú pháp `net.cpp` và `fish_tank.ino` bằng tệp giả của thư viện Arduino

Đây **không** thay cho biên dịch bằng Arduino IDE (T3.1), chỉ bắt lỗi chính tả, thiếu khai báo và sai kiểu trong các đoạn mới. Bước 1 tạo thư mục tệp giả, bước 2 kiểm tra cú pháp. Kết quả đạt: không in lỗi (`error`) nào.

```python
import os, textwrap
d = os.path.join(os.environ.get("TEMP", "/tmp"), "ft_stubs"); os.makedirs(d, exist_ok=True)
files = {
"Arduino.h": """#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
typedef uint8_t byte;
struct SerialStub { void begin(long) {} void println(const char*) {} void print(const char*) {} template<class T> void println(const T&) {} template<class T> void print(const T&) {} int printf(const char*, ...) __attribute__((format(printf,2,3))) { return 0; } void flush() {} };
static SerialStub Serial;
inline uint32_t millis() { return 0; }
inline void delay(uint32_t) {}
inline void pinMode(int,int) {}
inline void digitalWrite(int,int) {}
#define OUTPUT 1
#define HIGH 1
#define LOW 0
struct EspStub { void restart() {} }; static EspStub ESP;
""",
"esp_task_wdt.h": """#pragma once
#include <stdint.h>
typedef struct { uint32_t timeout_ms; uint32_t idle_core_mask; bool trigger_panic; } esp_task_wdt_config_t;
inline int esp_task_wdt_reconfigure(const esp_task_wdt_config_t*) { return 0; }
inline int esp_task_wdt_init(uint32_t, bool) { return 0; }
inline int esp_task_wdt_add(void*) { return 0; }
inline int esp_task_wdt_reset() { return 0; }
""",
"OneWire.h": "#pragma once\nstruct OneWire { OneWire(int) {} };\n",
"DallasTemperature.h": """#pragma once
#include "OneWire.h"
struct DallasTemperature { DallasTemperature(OneWire*) {} void begin() {} void setResolution(int) {} void setWaitForConversion(bool) {} void requestTemperatures() {} float getTempCByIndex(int) { return 0; } };
""",
"WiFi.h": """#pragma once
#include "Arduino.h"
enum { WL_CONNECTED = 3 }; enum { WIFI_STA = 1 };
struct IPAddr {};
struct WiFiStub { int status() { return 0; } void begin(const char*, const char*) {} void disconnect() {} void mode(int) {} void persistent(bool) {} void setAutoReconnect(bool) {} IPAddr localIP() { return IPAddr(); } int RSSI() { return 0; } };
static WiFiStub WiFi;
inline void configTime(long, int, const char*) {}
""",
"Client.h": "#pragma once\nstruct Client { virtual ~Client() {} };\n",
"WiFiClientSecure.h": """#pragma once
#include "Client.h"
struct WiFiClientSecure : Client { void setInsecure() {} void setTimeout(int) {} void setHandshakeTimeout(int) {} };
""",
"PubSubClient.h": """#pragma once
#include "Client.h"
#include <stdint.h>
#include <stddef.h>
struct PubSubClient {
  PubSubClient(Client&) {}
  PubSubClient& setServer(const char*, uint16_t) { return *this; }
  PubSubClient& setCallback(void (*)(char*, unsigned char*, unsigned int)) { return *this; }
  PubSubClient& setKeepAlive(uint16_t) { return *this; }
  PubSubClient& setSocketTimeout(uint16_t) { return *this; }
  bool setBufferSize(uint16_t) { return true; }
  bool connect(const char*, const char*, const char*, const char*, uint8_t, bool, const char*, bool) { return true; }
  bool connected() { return false; }
  bool subscribe(const char*, uint8_t = 0) { return true; }
  bool publish(const char*, const char*, bool = false) { return true; }
  bool loop() { return true; }
  void disconnect() {}
  int state() { return 0; }
};
""",
"ArduinoJson.h": """#pragma once
#include <stddef.h>
#include <stdint.h>
struct JsonVariantConst {
  template<typename T> bool is() const { return false; }
  template<typename T> T as() const { return T(); }
  bool isNull() const { return true; }
};
struct JsonObject {};
struct JsonObjectConst { JsonVariantConst operator[](const char*) const { return JsonVariantConst(); } };
struct JsonArrayConst { size_t size() const { return 0; } const JsonVariantConst* begin() const { return nullptr; } const JsonVariantConst* end() const { return nullptr; } };
struct JsonProxy { template<typename T> JsonProxy& operator=(T) { return *this; } };
struct JsonDocument { JsonProxy operator[](const char*) { return JsonProxy(); } template<typename T> bool is() const { return false; } template<typename T> T as() const { return T(); } };
struct DeserializationError { operator bool() const { return false; } };
inline DeserializationError deserializeJson(JsonDocument&, const char*, size_t) { return DeserializationError(); }
inline size_t serializeJson(const JsonDocument&, char*, size_t) { return 0; }
""",
"secrets.h": '#pragma once\n#define WIFI_SSID "x"\n#define WIFI_PASS "x"\n#define MQTT_PASS "x"\n',
}
for name, text in files.items():
    open(os.path.join(d, name), "w", encoding="utf-8").write(text)
print(d)
```

```bash
# thay <thu_muc_tep_gia> bằng đường dẫn mà kịch bản trên in ra
g++ -std=c++11 -fsyntax-only -Wall -DARDUINO -I<thu_muc_tep_gia> -Ifirmware/fish_tank firmware/fish_tank/net.cpp
g++ -std=c++11 -fsyntax-only -Wall -DARDUINO -I<thu_muc_tep_gia> -Ifirmware/fish_tank -x c++ firmware/fish_tank/fish_tank.ino
```

### 4.5 Rà soát sót chuỗi cũ và rà soát bí mật

```bash
# Không được còn kết quả nào ngoài các đoạn lịch sử trong docx và tệp fix-plan.md
grep -rn "to nhất liên tục" --include=*.h --include=*.cpp --include=*.ino --include=*.js --include=*.py --include=*.md . | grep -v fix-plan.md
grep -rn "08:00, 18:00" docs tools firmware | grep -v "Hai mốc: tự sắp"     # còn 1 dòng là tên ca thử (chủ ý), phần còn lại phải trống
grep -n "HYST_TEMP\|HYST_PH\|HYST_PCT" firmware/fish_tank/config.h           # 0.5f, 0.3f, 5.0f
git status --short                                                           # chỉ các tệp ở mục 2
git diff --stat -- firmware/fish_tank/secrets.h firmware/fish_tank/secrets.example.h   # phải trống
```

---

## 5. Việc nhóm phải tự làm (Agent không làm được, không giả vờ đã làm)

| # | Việc | Liên quan |
|---|---|---|
| 1 | Biên dịch và nạp bằng Arduino IDE (cần ArduinoJson 7, PubSubClient, ESP32Servo, OneWire, DallasTemperature; chọn board và core ESP32); dán Serial Monitor để làm bằng chứng T3.1 | Mọi phần firmware |
| 2 | Hiệu chuẩn pH, độ đục, mực nước; chốt `LEVEL_RAW_SHORT` (luôn lớn hơn `LEVEL_RAW_FULL`) và `LEVEL_SPREAD_MAX` bằng số đo thật; đặt `CALIBRATION_DONE = 1`; xác định chiều quay servo (`SERVO_FEED_US`) | Phase 1, T2.x |
| 3 | Trên EMQX: kiểm tra config retained cũ rồi bấm **Lưu** trên web hoặc publish `config-default.json`; xác nhận đã đổi mật khẩu WiFi và tài khoản `esp32` như đã báo; giữ Spend Limit bằng 0 | Phase 2 |
| 4 | Thử tiếng còi mức 3 (3 tiếng mỗi 5 giây), chọn duty bằng tai (T2.7, T3.9); thử nút Tắt tiếng bằng web thật (T3.12) | Phase 3 |
| 5 | Rút router để thử khởi động lại (T3.10), chạy liên tục 24 đến 48 giờ (T5.5, T5.7) | Phase 4 |
| 6 | Chạy lại ba bộ ca thử, chụp ảnh mới vào `tests/phase3/evidence/` rồi tự ghi `tests/phase3/results.md` | Phase 5 |
| 7 | Duyệt sáu giả định ở mục 1.5 | Phase 0 |

---

## 6. Rủi ro đã biết

- `analogReadMilliVolts` và ngưỡng lỗi mực nước chưa thử trên phần cứng; nếu `LEVEL_RAW_FULL` thật gần 4090 thì `LEVEL_RAW_SHORT` phải nâng lên (hiệu chuẩn xong mới chốt), nếu không mực nước đầy sẽ bị báo lỗi cảm biến.
- Tắt tiếng ở mức 3 im tới 30 phút theo quyết định của nhóm; nếu sau này muốn an toàn hơn, giới hạn riêng cho mức 3 là thay đổi nhỏ trong `Alarm`.
- `tryConnectMqtt()` có thể chặn tới 5 đến 8 giây khi mạng xấu; còi mức 1 và 2 có thể lỡ nhịp lúc đó (đã ghi nhận, không sửa trong kế hoạch này).
- `setInsecure()` vẫn còn đến Giai đoạn 6.
- Kịch bản kiểm tra cú pháp bằng tệp giả không bắt được lỗi thật của thư viện (đặc biệt API ArduinoJson 7 và core ESP32 3.x); phải biên dịch thật.
