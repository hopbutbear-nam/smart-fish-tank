#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
fake_esp32.py - ESP32 giả cho dự án Smart Fish Tank  (đặt tại tools/fake-esp32/)

Mục đích: thay ESP32 thật để thử web (GĐ4) và luồng MQTT khi chưa có linh kiện.
Bám theo kế hoạch: topic và quyền (E3), JSON (E4), giá trị mặc định (E6), luật cho ăn (G3).

Chạy từ thư mục gốc của kho (Windows):
    pip install -r tools/fake-esp32/requirements.txt
    PowerShell:  $env:MQTT_PASS = "mat-khau-esp32"
    CMD:         set MQTT_PASS=mat-khau-esp32
    python tools/fake-esp32/fake_esp32.py

Không đặt MQTT_PASS thì script sẽ hỏi mật khẩu (gõ không hiện ra màn hình).
TUYỆT ĐỐI không ghi mật khẩu vào file hay commit lên GitHub.

LƯU Ý: dùng chung client ID "esp32-tank01" với ESP32 thật. Khi ESP32 thật nối vào,
phải TẮT script này, nếu không hai bên sẽ đá nhau liên tục.
"""

import argparse
import getpass
import json
import os
import queue
import random
import re
import sys
import threading
import time
from pathlib import Path

import paho.mqtt.client as mqtt

# ----------------------------------------------------------------------------
# Hằng số theo kế hoạch
# ----------------------------------------------------------------------------
BROKER_HOST = "y132c53f.ala.asia-southeast1.emqxsl.com"   # E1
BROKER_PORT = 8883                                          # E1 (MQTT over TLS)
USERNAME = "esp32"                                          # E2
CLIENT_ID = "esp32-tank01"                                  # E2
KEEPALIVE_S = 30                                            # E6
FW_VERSION = "1.0.0-fake"   # hậu tố -fake để không nhầm với dữ liệu thật

DEFAULT_CONFIG = {                                          # E6
    "v": 1,
    "temp_min": 24.0, "temp_max": 30.0,
    "ph_min": 6.5, "ph_max": 8.0,
    "turb_max_pct": 40.0,
    "level_min_pct": 30.0, "level_danger_pct": 15.0,
    "buzzer_on": True, "mute_min": 30,
    "feed_ms": 600,
    "feed_times": [],
}

# ----------------------------------------------------------------------------
# GIẢ ĐỊNH CỦA SCRIPT - kế hoạch CHƯA quy định, nhóm cần chốt rồi sửa tại đây
# ----------------------------------------------------------------------------
DANGER_MARGIN_TEMP = 3.0    # vượt ngoài [temp_min, temp_max] quá 3 °C  -> risk 2
DANGER_MARGIN_PH = 1.0      # vượt ngoài [ph_min, ph_max] quá 1.0 pH    -> risk 2
DANGER_MARGIN_TURB = 20.0   # vượt turb_max_pct quá 20 điểm %           -> risk 2
SENSOR_FAULT_RISK = 1       # cảm biến lỗi (temp_c/ph = null)           -> risk 1
MAX_PORTIONS = 3            # số phần tối đa cho một lệnh (kế hoạch không giới hạn)
# Cách tính "24 giờ" ở đây là cửa sổ trượt 24 giờ gần nhất; firmware có thể khác.

# Chỉ để tạo số minh hoạ cho turb_v (0% -> 3.17 V, 100% -> 0.5 V; khớp ví dụ 12% ~ 2.85 V ở E4)
V_CLEAR, V_DIRTY = 3.17, 0.5

TIME_RE = re.compile(r"^([01]\d|2[0-3]):[0-5]\d$")


def log(msg):
    print(time.strftime("%H:%M:%S"), msg, flush=True)


def clamp(x, lo, hi):
    return max(lo, min(hi, x))


def is_num(x):
    return isinstance(x, (int, float)) and not isinstance(x, bool)


# ----------------------------------------------------------------------------
# Logic thuần (dễ kiểm tra riêng)
# ----------------------------------------------------------------------------
def evaluate(r, cfg):
    """Trả về (risk, flags) theo quy tắc ở G3: ngoài ngưỡng cảnh báo = 1, ngoài ngưỡng nguy hiểm = 2."""
    risk, flags = 0, []

    def mark(level, flag):
        nonlocal risk
        flags.append(flag)
        risk = max(risk, level)

    t = r["temp_c"]
    if t is None:
        mark(SENSOR_FAULT_RISK, "sensor_temp")
    elif t > cfg["temp_max"]:
        mark(2 if t > cfg["temp_max"] + DANGER_MARGIN_TEMP else 1, "temp_high")
    elif t < cfg["temp_min"]:
        mark(2 if t < cfg["temp_min"] - DANGER_MARGIN_TEMP else 1, "temp_low")

    p = r["ph"]
    if p is None:
        mark(SENSOR_FAULT_RISK, "sensor_ph")
    elif p > cfg["ph_max"]:
        mark(2 if p > cfg["ph_max"] + DANGER_MARGIN_PH else 1, "ph_high")
    elif p < cfg["ph_min"]:
        mark(2 if p < cfg["ph_min"] - DANGER_MARGIN_PH else 1, "ph_low")

    if r["turb_pct"] > cfg["turb_max_pct"]:
        mark(2 if r["turb_pct"] > cfg["turb_max_pct"] + DANGER_MARGIN_TURB else 1, "turb_high")

    if r["level_pct"] < cfg["level_danger_pct"]:
        mark(2, "level_low")
    elif r["level_pct"] < cfg["level_min_pct"]:
        mark(1, "level_low")

    return risk, flags


def validate_config(doc, current):
    """Trả về (cấu hình mới, thông báo lỗi). Sai bất kỳ giá trị nào thì từ chối cả bản tin."""
    if not isinstance(doc, dict):
        return None, "không phải đối tượng JSON"
    new = dict(current)
    for k in ("temp_min", "temp_max", "ph_min", "ph_max", "turb_max_pct",
              "level_min_pct", "level_danger_pct"):
        if k in doc:
            if not is_num(doc[k]):
                return None, f"{k} phải là số"
            new[k] = float(doc[k])
    if "feed_ms" in doc:
        if not is_num(doc["feed_ms"]) or not (100 <= doc["feed_ms"] <= 3000):
            return None, "feed_ms phải trong khoảng 100 đến 3000"
        new["feed_ms"] = int(doc["feed_ms"])
    if "mute_min" in doc:
        if not is_num(doc["mute_min"]) or not (0 <= doc["mute_min"] <= 1440):
            return None, "mute_min phải trong khoảng 0 đến 1440"
        new["mute_min"] = int(doc["mute_min"])
    if "buzzer_on" in doc:
        if not isinstance(doc["buzzer_on"], bool):
            return None, "buzzer_on phải là true/false"
        new["buzzer_on"] = doc["buzzer_on"]
    if "feed_times" in doc:
        ft = doc["feed_times"]
        if (not isinstance(ft, list) or len(ft) > 4
                or not all(isinstance(x, str) and TIME_RE.match(x) for x in ft)):
            return None, "feed_times phải là tối đa 4 mốc dạng HH:MM"
        new["feed_times"] = ft
    if "v" in doc and is_num(doc["v"]):
        new["v"] = int(doc["v"])
    if not new["temp_min"] < new["temp_max"]:
        return None, "temp_min phải nhỏ hơn temp_max"
    if not new["ph_min"] < new["ph_max"]:
        return None, "ph_min phải nhỏ hơn ph_max"
    if not (0 <= new["level_danger_pct"] <= new["level_min_pct"] <= 100):
        return None, "cần 0 <= level_danger_pct <= level_min_pct <= 100"
    if not (0 < new["turb_max_pct"] <= 100):
        return None, "turb_max_pct phải trong (0, 100]"
    return new, None


# ----------------------------------------------------------------------------
# ESP32 giả
# ----------------------------------------------------------------------------
class FakeEsp32:
    HELP = """Lệnh (gõ rồi Enter):
  status                 xem trạng thái hiện tại
  normal                 đưa mọi giá trị về bình thường
  hot | cold             nhiệt độ vượt ngưỡng (cảnh báo)
  phlow | phhigh         pH vượt ngưỡng (cảnh báo)
  turbid                 nước đục (cảnh báo)
  lowwater | dry         mực nước thấp (cảnh báo) | rất thấp (nguy hiểm)
  set <temp|ph|turb|level> <số>   đặt giá trị tuỳ ý, ví dụ: set temp 35
  fail <temp|ph>         giả lỗi cảm biến (giá trị null + cờ sensor_*)
  fix                    sửa mọi lỗi cảm biến
  boot                   gửi lại sự kiện boot
  crash                  thoát đột ngột (không DISCONNECT) -> broker gửi Last Will "offline"
  quit                   thoát êm: tự gửi "offline" rồi ngắt
  help                   in bảng này"""

    def __init__(self, args, password):
        self.args = args
        self.tank = args.tank
        self.t_tele = f"fishtank/{self.tank}/telemetry"
        self.t_status = f"fishtank/{self.tank}/status"
        self.t_event = f"fishtank/{self.tank}/event"
        self.t_cmd = f"fishtank/{self.tank}/cmd/#"
        self.t_feed = f"fishtank/{self.tank}/cmd/feed"
        self.t_config = f"fishtank/{self.tank}/config"

        self.q = queue.Queue()
        self.cfg = dict(DEFAULT_CONFIG)
        self.base = {"temp": 27.5, "ph": 7.12, "turb": 12.0, "level": 80.0}
        self.fail = set()
        self.reading = None
        self.risk, self.flags = 0, []
        self.t0 = time.monotonic()

        self.connected = False
        self.stop = False
        self.force_publish = True
        self.next_sample = 0.0
        self.last_publish = 0.0

        self.last_feed_id = ""
        self.feeding = None            # {"id","portions","end"}
        self.last_feed_done = None     # monotonic
        self.feeds = []                # các lần cho ăn xong trong 24 giờ gần nhất

        self.client = self._make_client(password)

    # ---- MQTT ----
    def _make_client(self, password):
        if hasattr(mqtt, "CallbackAPIVersion"):                       # paho-mqtt 2.x
            c = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=CLIENT_ID, clean_session=True)
        else:                                                         # paho-mqtt 1.x
            c = mqtt.Client(client_id=CLIENT_ID, clean_session=True)
        c.username_pw_set(self.args.user, password)
        c.tls_set(ca_certs=str(self.args.ca))          # bắt buộc xác minh chứng chỉ máy chủ
        c.will_set(self.t_status, "offline", qos=1, retain=True)      # Last Will (E6)
        c.reconnect_delay_set(min_delay=1, max_delay=30)               # nối lại 1..30 giây
        c.on_connect = self.on_connect
        c.on_disconnect = self.on_disconnect
        c.on_message = self.on_message
        c.on_connect_fail = self.on_connect_fail
        if self.args.debug:
            import logging
            logging.basicConfig(level=logging.DEBUG)
            c.enable_logger()
        return c

    def on_connect(self, client, userdata, flags, rc, props=None):
        failed = rc.is_failure if hasattr(rc, "is_failure") else (rc != 0)
        if failed:
            log(f"[LỖI] Broker từ chối kết nối ({rc}). Kiểm tra username/mật khẩu. Dừng script.")
            self.stop = True
            return
        self.connected = True
        log("Đã kết nối EMQX (TLS, đã xác minh chứng chỉ).")
        client.subscribe([(self.t_cmd, 1), (self.t_config, 1)])
        # ESP32 thật dùng PubSubClient nên chỉ publish được QoS 0 (ghi chú ở E3);
        # chỉ Last Will mới là QoS 1. Giả lập đúng như vậy.
        client.publish(self.t_status, "online", qos=0, retain=True)
        self.publish_event(type="boot", fw=FW_VERSION, ts=int(time.time()))
        self.force_publish = True

    def on_disconnect(self, client, userdata, *args):
        if self.connected and not self.stop:
            log("Mất kết nối. Đang tự nối lại (chờ 1 đến 30 giây)...")
        self.connected = False

    def on_connect_fail(self, client, userdata):
        log("Nối chưa được (mạng, tên miền hoặc chứng chỉ CA?). Sẽ thử lại...")

    def on_message(self, client, userdata, msg):
        self.q.put(("msg", msg.topic, msg.payload))

    def pub(self, topic, obj, qos=0, retain=False):
        payload = json.dumps(obj, separators=(",", ":"), ensure_ascii=False)
        self.client.publish(topic, payload, qos=qos, retain=retain)

    def publish_event(self, **fields):
        self.pub(self.t_event, fields, qos=0, retain=False)

    # ---- Cảm biến giả ----
    def sample(self):
        b = self.base
        temp = None if "temp" in self.fail else round(b["temp"] + random.gauss(0, 0.05), 1)
        ph = None if "ph" in self.fail else round(b["ph"] + random.gauss(0, 0.02), 2)
        turb = clamp(b["turb"] + random.gauss(0, 0.4), 0, 100)
        level = clamp(b["level"] + random.gauss(0, 0.4), 0, 100)
        return {
            "temp_c": temp, "ph": ph,
            "turb_v": round(V_CLEAR - (V_CLEAR - V_DIRTY) * turb / 100.0, 2),
            "turb_pct": int(round(turb)), "level_pct": int(round(level)),
        }

    def publish_telemetry(self):
        r = self.reading
        doc = {
            "v": 1, "ts": int(time.time()),
            "temp_c": r["temp_c"], "ph": r["ph"], "turb_v": r["turb_v"],
            "turb_pct": r["turb_pct"], "level_pct": r["level_pct"],
            "risk": self.risk, "flags": self.flags,
            "rssi": int(round(random.gauss(-61, 2))),
            "uptime_s": int(time.monotonic() - self.t0),
            "fw": FW_VERSION,
        }
        if self.args.omit_null:
            doc = {k: v for k, v in doc.items() if v is not None}
        self.pub(self.t_tele, doc, qos=0, retain=True)

    # ---- Xử lý tin nhận được ----
    def handle_message(self, topic, payload):
        try:
            doc = json.loads(payload)
        except (ValueError, UnicodeDecodeError):
            log(f"Bỏ qua tin JSON lỗi ở {topic}")
            return
        if topic == self.t_feed:
            self.handle_feed(doc)
        elif topic == self.t_config:
            self.handle_config(doc)

    def handle_config(self, doc):
        new, err = validate_config(doc, self.cfg)
        if err:
            log(f"[config] TỪ CHỐI: {err}")
            return
        self.cfg = new
        self.force_publish = True
        log(f"[config] đã áp dụng: {json.dumps(new, ensure_ascii=False)}")

    def reject(self, fid, reason):
        log(f"[cho ăn] từ chối id={fid}: {reason}")
        self.publish_event(type="feed_rejected", id=fid, reason=reason)

    def handle_feed(self, doc):
        if not isinstance(doc, dict):
            return
        fid = str(doc.get("id", ""))
        portions = doc.get("portions", 1)
        if not is_num(portions) or portions < 1:
            portions = 1
        portions = min(int(portions), MAX_PORTIONS)

        if fid == self.last_feed_id:                    # chống lệnh lặp (G3)
            log(f"[cho ăn] bỏ qua lệnh trùng id={fid}")
            return
        self.last_feed_id = fid

        now = time.monotonic()
        if self.feeding:
            return self.reject(fid, "busy")
        if (self.last_feed_done is not None
                and now - self.last_feed_done < self.args.min_interval):
            return self.reject(fid, "too_soon")
        self.feeds = [t for t in self.feeds if now - t < 86400]
        if len(self.feeds) >= self.args.max_per_day:
            return self.reject(fid, "daily_limit")

        dur = self.cfg["feed_ms"] * portions / 1000.0
        self.feeding = {"id": fid, "portions": portions, "end": now + dur}
        log(f"[cho ăn] bắt đầu id={fid}, {portions} phần, quay {dur:.1f} giây")

    # ---- Lệnh gõ tay ----
    def handle_command(self, line):
        p = line.strip().split()
        if not p:
            return
        c = p[0].lower()
        presets = {
            "normal": {"temp": 27.5, "ph": 7.12, "turb": 12.0, "level": 80.0},
            "hot": {"temp": 31.5}, "cold": {"temp": 22.5},
            "phlow": {"ph": 6.2}, "phhigh": {"ph": 8.4},
            "turbid": {"turb": 55.0},
            "lowwater": {"level": 25.0}, "dry": {"level": 10.0},
        }
        if c in presets:
            self.base.update(presets[c])
            if c == "normal":
                self.fail.clear()
            self.force_publish = True
            log(f"OK: {c}")
        elif c == "set" and len(p) == 3 and p[1] in self.base:
            try:
                self.base[p[1]] = float(p[2])
                self.force_publish = True
                log(f"OK: {p[1]} = {p[2]}")
            except ValueError:
                log("Giá trị phải là số. Ví dụ: set temp 35")
        elif c == "fail" and len(p) == 2 and p[1] in ("temp", "ph"):
            self.fail.add(p[1])
            self.force_publish = True
            log(f"OK: giả lỗi cảm biến {p[1]}")
        elif c == "fix":
            self.fail.clear()
            self.force_publish = True
            log("OK: đã sửa cảm biến")
        elif c == "boot":
            self.publish_event(type="boot", fw=FW_VERSION, ts=int(time.time()))
        elif c == "status":
            self.print_status()
        elif c == "crash":
            log("Thoát đột ngột (không DISCONNECT) - broker sẽ gửi Last Will.")
            os._exit(1)
        elif c == "quit":
            self.stop = True
        elif c == "help":
            print(self.HELP, flush=True)
        else:
            log("Lệnh không hiểu. Gõ 'help' để xem danh sách.")

    def print_status(self):
        r = self.reading or {}
        feeding = f"đang cho ăn id={self.feeding['id']}" if self.feeding else "rảnh"
        log(f"kết nối={'có' if self.connected else 'KHÔNG'} | risk={self.risk} flags={self.flags} | "
            f"giá trị={r} | cho ăn: {feeding}, {len(self.feeds)} lần trong 24h "
            f"(tối đa {self.args.max_per_day}, cách nhau {self.args.min_interval}s)")
        log(f"config hiện tại: {json.dumps(self.cfg, ensure_ascii=False)}")

    # ---- Vòng lặp chính (một luồng xử lý logic, không cần khoá) ----
    def tick(self):
        now = time.monotonic()
        if self.feeding and now >= self.feeding["end"]:
            f, self.feeding = self.feeding, None
            self.last_feed_done = now
            self.feeds.append(now)
            log(f"[cho ăn] xong id={f['id']}")
            self.publish_event(type="feed_done", id=f["id"], portions=f["portions"], ts=int(time.time()))

        if self.force_publish or now >= self.next_sample:
            self.reading = self.sample()
            risk, flags = evaluate(self.reading, self.cfg)
            changed = (risk != self.risk) or (flags != self.flags)
            if risk > self.risk and self.connected:
                self.publish_event(type="alarm", level=risk, msg=flags[0], ts=int(time.time()))
            self.risk, self.flags = risk, flags
            self.next_sample = now + 2.0                 # đọc mỗi 2 giây
            due = now - self.last_publish >= self.args.interval
            if self.connected and (self.force_publish or changed or due):
                self.publish_telemetry()
                self.last_publish = now
                self.force_publish = False
                if changed:
                    log(f"risk={risk} flags={flags}")

    def read_stdin(self):
        for line in sys.stdin:
            self.q.put(("cmd", line))

    def run(self):
        log(f"Kết nối tới {self.args.host}:{self.args.port} bằng tài khoản '{self.args.user}' ...")
        self.client.connect_async(self.args.host, self.args.port, KEEPALIVE_S)
        self.client.loop_start()
        threading.Thread(target=self.read_stdin, daemon=True).start()
        print(self.HELP, flush=True)
        try:
            while not self.stop:
                try:
                    item = self.q.get(timeout=0.2)
                    if item[0] == "msg":
                        self.handle_message(item[1], item[2])
                    else:
                        self.handle_command(item[1])
                except queue.Empty:
                    pass
                self.tick()
        except KeyboardInterrupt:
            pass
        self.shutdown()

    def shutdown(self):
        # Ngắt êm KHÔNG kích hoạt Last Will, nên phải tự báo offline (retained)
        if self.connected:
            info = self.client.publish(self.t_status, "offline", qos=0, retain=True)
            info.wait_for_publish(timeout=3)
        self.client.disconnect()
        self.client.loop_stop()
        log("Đã thoát.")


def main():
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass
    default_ca = Path(__file__).resolve().parents[2] / "firmware" / "certs" / "emqx-ca.pem"
    ap = argparse.ArgumentParser(description="ESP32 giả cho Smart Fish Tank")
    ap.add_argument("--host", default=BROKER_HOST)
    ap.add_argument("--port", type=int, default=BROKER_PORT)
    ap.add_argument("--user", default=USERNAME)
    ap.add_argument("--tank", default="tank01", help="mã bể trong topic (mặc định tank01)")
    ap.add_argument("--ca", type=Path, default=default_ca, help="đường dẫn emqx-ca.pem")
    ap.add_argument("--interval", type=float, default=10.0, help="chu kỳ gửi telemetry (giây)")
    ap.add_argument("--min-interval", type=float, default=60.0,
                    help="giãn cách tối thiểu giữa hai lần cho ăn (giây); hạ xuống để thử nhanh")
    ap.add_argument("--max-per-day", type=int, default=4, help="số lần cho ăn tối đa mỗi 24 giờ")
    ap.add_argument("--omit-null", action="store_true",
                    help="bỏ hẳn trường khi cảm biến lỗi (thay vì để null)")
    ap.add_argument("--debug", action="store_true", help="in log chi tiết của paho")
    args = ap.parse_args()

    if not args.ca.is_file():
        sys.exit(f"Không thấy file CA: {args.ca}\nHãy đặt emqx-ca.pem vào firmware/certs/ hoặc dùng --ca.")
    password = os.environ.get("MQTT_PASS") or getpass.getpass(f"Mật khẩu của '{args.user}': ")

    FakeEsp32(args, password).run()


if __name__ == "__main__":
    main()
