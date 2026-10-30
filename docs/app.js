"use strict";

/* ====== CẤU HÌNH (khớp mục E của kế hoạch bản 1.2) ====== */
const BROKER = "wss://y132c53f.ala.asia-southeast1.emqxsl.com:8084/mqtt";
const TANK = "tank01";
const TOPIC = {
    telemetry: `fishtank/${TANK}/telemetry`,
    status: `fishtank/${TANK}/status`,
    event: `fishtank/${TANK}/event`,
    feed: `fishtank/${TANK}/cmd/feed`,
    config: `fishtank/${TANK}/config`,
};

const HIST_KEY = "ft_hist";
const EVT_KEY = "ft_events";
const HIST_MAX_MS = 24 * 3600 * 1000;      // giữ biểu đồ 24 giờ
const HIST_STEP_MS = 30000;                // 1 điểm biểu đồ mỗi 30 giây
const GAP_MS = 120000;                     // hở quá 2 phút thì ngắt nét vẽ
const STALE_MS = 30000;                    // quá 30 giây không có telemetry mới = dữ liệu cũ (mục E3, D8)
const EVT_MAX = 100;                       // số sự kiện tối đa lưu trong trình duyệt
const EVT_MAX_AGE = 7 * 24 * 3600 * 1000;  // giữ sự kiện 7 ngày
const EVT_SHOW = 50;                       // số sự kiện hiển thị
const ALARM_GAP_MS = { 1: 60000, 2: 30000, 3: 15000 };   // giãn cách tối thiểu giữa hai dòng cảnh báo CÙNG MỨC: mức 1 = 60 giây, mức 2 = 30 giây, mức 3 = 15 giây
const FEED_WAIT_MS = 15000;                // chờ phản hồi cho ăn tối đa 15 giây
const MAX_FEED_TIMES = 3;                  // D6 (đề xuất): tối đa 3 mốc, chừa 1 lần cho "Cho ăn ngay". Đổi thành 4 nếu nhóm chốt 4.

const LIMIT = { tempMin: 18, tempMax: 33, phMin: 5.0, phMax: 8.5 };   // biên nguy hiểm (mục E6, E8)
const L2_ADD = { temp: 2, turb: 15 };                                  // mức 2 = ngưỡng mức 1 cộng thêm (mục E8)

// Giá trị mặc định (mục E4, E6). Dùng khi chưa nhận được config retained.
const DEFAULT_CFG = {
    temp_min: 23, temp_max: 29, ph_min: 6.0, ph_max: 8.0,
    turb_max_pct: 30, level_min_pct: 30, level_danger_pct: 15,
    mute_min: 30, feed_ms: 600, buzzer_on: true, feed_times: ["08:00", "18:00"],
};

const $ = (id) => document.getElementById(id);

const FLAG_TEXT = {
    temp_high: "nhiệt độ cao", temp_low: "nhiệt độ thấp",
    ph_low: "pH thấp", ph_high: "pH cao",
    turb_high: "nước đục", level_low: "mực nước thấp",
    sensor_temp: "lỗi cảm biến nhiệt độ", sensor_ph: "lỗi cảm biến pH",
};
const REASON_TEXT = {
    too_soon: "quá sớm (phải chờ ít nhất 60 giây giữa hai lần)",
    daily_limit: "đã đủ số lần trong 24 giờ qua",
    busy: "đang cho ăn",
};
const CFG_FIELDS = {
    temp_min: "s_tmin", temp_max: "s_tmax", ph_min: "s_pmin", ph_max: "s_pmax",
    turb_max_pct: "s_turb", level_min_pct: "s_lmin", level_danger_pct: "s_ldanger",
    mute_min: "s_mute", feed_ms: "s_feedms",
};

/* ====== TRẠNG THÁI ====== */
let client = null;
let activeCfg = Object.assign({}, DEFAULT_CFG);   // ngưỡng đang áp dụng (từ config retained)
let haveData = false;       // đã nhận ít nhất 1 telemetry
let lastTs = 0;             // thời điểm (ms) của telemetry gần nhất; 0 = chưa rõ
let lastTele = null;        // telemetry gần nhất (để vẽ lại khi đổi ngưỡng hoặc đổi giao diện sáng/tối)
let offlineFlag = false;    // true khi nhận status "offline"; telemetry sống (không retained) sẽ xóa cờ này
let prevOnline = null;      // để ghi sự kiện online/offline khi trạng thái đổi
let wentOffline = false;
let pendingFeed = null;
let feedTimer = null;
let history = [];
let lastHistT = 0;
let events = [];
let chartsReady = false;
let pluginRegistered = false;
const charts = {};

/* ====== TIỆN ÍCH ====== */
function clamp(x, a, b) { return Math.min(b, Math.max(a, x)); }
function lerp(a, b, t) { return a + (b - a) * t; }
function num(x) { return (typeof x === "number" && isFinite(x)) ? x : null; }
function hsl(h, s, l) { return `hsl(${Math.round(h)}, ${Math.round(s)}%, ${Math.round(l)}%)`; }
function interp(stops, x) {
    if (x <= stops[0][0]) return stops[0][1];
    for (let i = 1; i < stops.length; i++) {
        if (x <= stops[i][0]) {
            const [x0, y0] = stops[i - 1], [x1, y1] = stops[i];
            return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
        }
    }
    return stops[stops.length - 1][1];
}
function isDark() {
    try { return !!(window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches); }
    catch (e) { return false; }
}
function fmtClock(ms) { return new Date(ms).toLocaleTimeString("vi-VN", { hour: "2-digit", minute: "2-digit", hour12: false }); }
function fmtTime(ms) { return new Date(ms).toLocaleTimeString("vi-VN", { hour12: false }); }
function fmtDateTime(ms) {
    return new Date(ms).toLocaleString("vi-VN", { day: "2-digit", month: "2-digit", hour: "2-digit", minute: "2-digit", second: "2-digit", hour12: false });
}
function flagListText(msg) {
    const arr = Array.isArray(msg) ? msg : String(msg || "").split(",");
    return arr.map(s => String(s).trim()).filter(Boolean).map(f => FLAG_TEXT[f] || f).join(", ");
}

/* ====== LƯU TRỮ (bọc try/catch) ====== */
function ssGet(k) { try { return sessionStorage.getItem(k); } catch (e) { return null; } }
function ssSet(k, v) { try { sessionStorage.setItem(k, v); } catch (e) { } }
function ssDel(k) { try { sessionStorage.removeItem(k); } catch (e) { } }

function loadHistory() {
    try {
        const a = JSON.parse(localStorage.getItem(HIST_KEY) || "[]");
        const cut = Date.now() - HIST_MAX_MS;
        return Array.isArray(a) ? a.filter(p => p && typeof p.t === "number" && p.t > cut) : [];
    } catch (e) { return []; }
}
function saveHistory() { try { localStorage.setItem(HIST_KEY, JSON.stringify(history)); } catch (e) { } }

function loadEvents() {
    try {
        const a = JSON.parse(localStorage.getItem(EVT_KEY) || "[]");
        const cut = Date.now() - EVT_MAX_AGE;
        return Array.isArray(a)
            ? a.filter(x => x && typeof x.t === "number" && typeof x.k === "string" && typeof x.txt === "string" && x.t > cut)
            : [];
    } catch (e) { return []; }
}
function persistEvents(list) { try { localStorage.setItem(EVT_KEY, JSON.stringify(list)); } catch (e) { } }

/* ====== LỊCH SỬ SỰ KIỆN ====== */
// Thiết bị gửi event QoS 0, không retained: broker không giữ lại. Vì vậy web tự lưu vào localStorage.
// Gộp với bản đã lưu (các tab khác có thể vừa ghi) và loại trùng theo khóa k.
function pushEvent(x) {
    const map = new Map();
    loadEvents().forEach(y => map.set(y.k, y));
    events.forEach(y => map.set(y.k, y));
    if (!map.has(x.k)) map.set(x.k, x);
    const list = Array.from(map.values()).sort((a, b) => b.t - a.t).slice(0, EVT_MAX);
    events = list;
    persistEvents(list);
    renderEvents();
}
function renderEvents() {
    const ul = $("events");
    ul.textContent = "";
    if (!events.length) {
        const li = document.createElement("li");
        li.className = "empty";
        li.textContent = "Chưa có sự kiện nào.";
        ul.appendChild(li);
        return;
    }
    events.slice(0, EVT_SHOW).forEach(x => {
        const li = document.createElement("li");
        li.className = x.cls || "";
        const t = document.createElement("span");
        t.className = "time";
        t.textContent = fmtDateTime(x.t);
        const m = document.createElement("span");
        m.className = "txt";
        m.textContent = x.txt;
        li.appendChild(t);
        li.appendChild(m);
        ul.appendChild(li);
    });
}
function logLocal(txt, cls, keyPart) {
    const now = Date.now();
    pushEvent({ k: "local|" + (keyPart || (now + "|" + Math.random().toString(36).slice(2, 6))), t: now, cls: cls || "ev-local", txt });
}

/* ====== TIỆN ÍCH GIAO DIỆN ====== */
function showMsg(id, text, cls) { const e = $(id); e.textContent = text; e.className = "msg " + (cls || ""); }
function setConn(text, cls) { const e = $("conn"); e.textContent = text; e.className = "badge " + cls; }
function showLogin(msg) {
    $("app").hidden = true; $("login").hidden = false;
    showMsg("loginMsg", msg || "", msg ? "bad" : "");
}
function showApp() {
    $("login").hidden = true; $("app").hidden = false;
    if (!chartsReady) { initCharts(); chartsReady = true; }
}

/* ====== MỨC CẢNH BÁO TỪNG CHỈ SỐ (bảng mục E8) ====== */
// So sánh nghiêm ngặt (vượt quá mới tính).
function sevTemp(v, c) {
    if (v < LIMIT.tempMin || v > LIMIT.tempMax) return 3;
    if (v < c.temp_min - L2_ADD.temp || v > c.temp_max + L2_ADD.temp) return 2;
    if (v < c.temp_min || v > c.temp_max) return 1;
    return 0;
}
function sevPh(v, c) {                       // pH chỉ có mức 1 và 2
    if (v < LIMIT.phMin || v > LIMIT.phMax) return 2;
    if (v < c.ph_min || v > c.ph_max) return 1;
    return 0;
}
function sevTurb(v, c) {
    if (v > c.turb_max_pct + L2_ADD.turb) return 2;
    if (v > c.turb_max_pct) return 1;
    return 0;
}
function sevLevel(v, c) {
    if (v < c.level_danger_pct) return 2;
    if (v < c.level_min_pct) return 1;
    return 0;
}

/* ====== MÀU THEO TỪNG CHỈ SỐ ====== */
const PH_HUE = [[4, 0], [5, 18], [6, 48], [6.6, 85], [7, 135], [7.4, 160], [8, 185], [8.6, 215], [9.5, 255], [11, 280]];

function mkTone(h, s, lv, dark) {
    const boost = lv >= 2 ? 1 : 0;
    return dark
        ? { bg: hsl(h, s * 0.6, 19 + 3 * boost), img: "none", fg: hsl(h, Math.min(95, s + 25), 78), sub: hsl(h, 25, 72) }
        : { bg: hsl(h, s, 93 - 5 * boost), img: "none", fg: hsl(h, Math.min(90, s + 15), 25), sub: hsl(h, 30, 33) };
}
// Nhiệt độ: lạnh -> xanh dương, vừa -> xanh ngọc đến xanh lá, nóng -> vàng cam đỏ
function toneTemp(v, c, lv, dark) {
    let h, s;
    if (v < c.temp_min) {
        const t = clamp((c.temp_min - v) / Math.max(c.temp_min - LIMIT.tempMin, 1), 0, 1);
        h = lerp(200, 228, t); s = lerp(55, 85, t);
    } else if (v > c.temp_max) {
        const t = clamp((v - c.temp_max) / Math.max(LIMIT.tempMax - c.temp_max, 1), 0, 1);
        h = lerp(42, 2, t); s = lerp(65, 92, t);
    } else {
        const t = clamp((v - c.temp_min) / Math.max(c.temp_max - c.temp_min, 0.1), 0, 1);
        h = lerp(180, 95, t); s = 40;
    }
    return mkTone(h, s, lv, dark);
}
// pH: theo màu giấy chỉ thị (chua: đỏ cam, trung tính: xanh lá, kiềm: xanh dương tím)
function tonePh(v, c, lv, dark) {
    return mkTone(interp(PH_HUE, v), lv >= 1 ? 72 : 52, lv, dark);
}
// Độ đục: trong -> xanh ngọc nhạt; hơi đục -> xám nhạt; quá đục (có cảnh báo) -> nền xám đậm, chữ sáng
function toneTurb(v, c, lv, dark) {
    if (lv >= 1) {
        return { bg: hsl(210, 8, lv >= 2 ? 32 : 46), img: "none", fg: "#ffffff", sub: "#e6ebef" };
    }
    const t = clamp(v / Math.max(c.turb_max_pct, 1), 0, 1);
    const h = lerp(190, 208, t);
    return dark
        ? { bg: hsl(h, lerp(45, 12, t), lerp(19, 28, t)), img: "none", fg: hsl(h, lerp(70, 15, t), 80), sub: hsl(h, 20, 72) }
        : { bg: hsl(h, lerp(75, 18, t), lerp(93, 84, t)), img: "none", fg: hsl(h, lerp(70, 15, t), 24), sub: hsl(h, 30, 33) };
}
// Mực nước: nền là "thước đo" nước dâng theo %; bình thường xanh nước, mức 1 hổ phách, mức 2 đỏ
function toneLevel(v, c, lv, dark) {
    const pct = clamp(v, 0, 100);
    const water = lv >= 2 ? (dark ? hsl(2, 60, 38) : hsl(2, 80, 76))
        : lv === 1 ? (dark ? hsl(38, 70, 34) : hsl(40, 95, 72))
            : (dark ? hsl(205, 70, 32) : hsl(205, 85, 81));
    const base = dark ? hsl(205, 30, 15) : hsl(205, 60, 96);
    const fg = lv >= 2 ? (dark ? hsl(0, 90, 84) : hsl(0, 70, 28))
        : lv === 1 ? (dark ? hsl(40, 90, 80) : hsl(35, 80, 22))
            : (dark ? hsl(205, 60, 86) : hsl(205, 70, 20));
    return {
        bg: base,
        img: `linear-gradient(to top, ${water} 0, ${water} ${pct}%, ${base} ${pct}%, ${base} 100%)`,
        fg, sub: dark ? hsl(205, 25, 75) : hsl(205, 35, 30),
    };
}

const METRICS = [
    {
        key: "temp", get: d => d.temp_c, digits: 1, unit: " °C", flags: ["temp_high", "temp_low"], sensor: "sensor_temp",
        sev: sevTemp, tone: toneTemp,
        word: (v, c, f) => (f && f.includes("temp_low")) ? "Quá lạnh" : (f && f.includes("temp_high")) ? "Quá nóng" : (v < c.temp_min ? "Quá lạnh" : "Quá nóng"),
    },
    {
        key: "ph", get: d => d.ph, digits: 2, unit: "", flags: ["ph_low", "ph_high"], sensor: "sensor_ph",
        sev: sevPh, tone: tonePh,
        word: (v, c, f) => (f && f.includes("ph_low")) ? "pH thấp (nước chua)" : (f && f.includes("ph_high")) ? "pH cao (nước kiềm)" : (v < c.ph_min ? "pH thấp (nước chua)" : "pH cao (nước kiềm)"),
    },
    {
        key: "turb", get: d => d.turb_pct, digits: 0, unit: " %", flags: ["turb_high"], sensor: null,
        sev: sevTurb, tone: toneTurb,
        word: () => "Nước đục",
    },
    {
        key: "level", get: d => d.level_pct, digits: 0, unit: " %", flags: ["level_low"], sensor: null,
        sev: sevLevel, tone: toneLevel,
        word: (v, c, f, lv) => lv >= 2 ? "Mực nước rất thấp" : "Mực nước thấp",
    },
];

// Mức của một thẻ: firmware là nguồn sự thật về cờ (có làm trễ 3 lần đọc, mục D3); web chỉ tính độ nặng từ số đo.
function metricLevel(m, v, flags, c) {
    if (m.sensor && flags && flags.includes(m.sensor)) return 1;
    const sev = v === null ? 0 : m.sev(v, c);
    if (!flags) return sev;                              // thiết bị không gửi flags: dùng số đo
    return m.flags.some(f => flags.includes(f)) ? Math.max(1, sev) : 0;
}

function applyTone(card, t) {
    if (!t) {
        ["--c-bg", "--c-img", "--c-fg", "--c-sub"].forEach(p => card.style.removeProperty(p));
        return;
    }
    card.style.setProperty("--c-bg", t.bg);
    card.style.setProperty("--c-img", t.img || "none");
    card.style.setProperty("--c-fg", t.fg);
    card.style.setProperty("--c-sub", t.sub);
}

function renderCards(d) {
    const c = activeCfg, dark = isDark();
    const flags = Array.isArray(d.flags) ? d.flags : null;
    const levels = [];
    METRICS.forEach(m => {
        const v = num(m.get(d));
        const sensorBad = !!(m.sensor && flags && flags.includes(m.sensor));
        const lv = metricLevel(m, v, flags, c);
        levels.push(lv);
        const card = $("c_" + m.key), chip = $("n_" + m.key);
        $("v_" + m.key).textContent = v === null ? "--" : v.toFixed(m.digits) + m.unit;
        if (v === null) {
            const bad = sensorBad;
            card.className = "card " + (bad ? "lv1" : "nodata");
            chip.className = "chip " + (bad ? "lv1" : "nodata");
            chip.textContent = bad ? "Mức 1 · Lỗi cảm biến" : "Chưa có số";
            applyTone(card, null);
            return;
        }
        card.className = "card lv" + lv;
        chip.className = "chip lv" + lv;
        chip.textContent = lv === 0 ? "Bình thường" : sensorBad ? "Mức 1 · Lỗi cảm biến" : "Mức " + lv + " · " + m.word(v, c, flags, lv);
        applyTone(card, m.tone(v, c, lv, dark));
    });
    return levels;
}

function overallRisk(d, levels) {
    const r = Number(d.risk);
    if (Number.isFinite(r)) return clamp(Math.round(r), 0, 3);
    return Math.max(0, ...levels);
}

function renderAlert(risk, d) {
    const al = $("alert"), rb = $("riskBadge");
    rb.textContent = risk === 0 ? "Cảnh báo: bình thường" : "Cảnh báo: mức " + risk;
    rb.className = "badge lv" + risk;
    document.title = risk >= 1 ? `⚠ Mức ${risk} · Bể cá thông minh` : "Bể cá thông minh";
    if (risk < 1) { al.hidden = true; return; }
    const names = flagListText(Array.isArray(d.flags) ? d.flags : []);
    const head = risk >= 3 ? "🚨 MỨC 3 · NGUY HIỂM" : risk === 2 ? "⚠ MỨC 2 · Cảnh báo cao" : "⚠ Mức 1 · Cảnh báo";
    al.textContent = head + ": " + (names || "có chỉ số bất thường");
    al.className = "alert lv" + risk;
    al.hidden = false;
}

/* ====== BIỂU ĐỒ ====== */
const CHART_DEFS = [
    { id: "chart_temp", key: "temp", title: "Nhiệt độ (°C)", color: "#e8743b", min: 22, max: 32 },
    { id: "chart_ph", key: "ph", title: "pH", color: "#7b5ea7", min: 5, max: 9 },
    { id: "chart_turb", key: "turb", title: "Độ đục (%)", color: "#7a8591", min: 0, max: 50 },
    { id: "chart_level", key: "level", title: "Mực nước (%)", color: "#2d8fd5", min: 0, max: 100 },
];

// Màu từng đoạn đường theo giá trị
function segColor(key, y) {
    if (typeof y !== "number" || !isFinite(y)) return undefined;
    const c = activeCfg;
    if (key === "temp") return y < c.temp_min ? "#2f7fd8" : y > c.temp_max ? "#e5502b" : "#2aa876";
    if (key === "ph") return y < c.ph_min ? "#e0662f" : y > c.ph_max ? "#3b6fd1" : "#2aa876";
    if (key === "turb") return y > c.turb_max_pct ? "#7a8591" : "#26a6c9";
    if (key === "level") return y < c.level_danger_pct ? "#d62839" : y < c.level_min_pct ? "#f07a1a" : "#2d8fd5";
    return undefined;
}

// Vùng nền xanh: nhiệt độ thích hợp 25 đến 27 °C (mục Giai đoạn 4); các chỉ số khác tô vùng bình thường
const BAND_FN = {
    chart_temp: () => [{ from: 25, to: 27, color: "rgba(42,168,118,.20)" }],
    chart_ph: () => [{ from: activeCfg.ph_min, to: activeCfg.ph_max, color: "rgba(42,168,118,.12)" }],
    chart_turb: () => [{ from: 0, to: activeCfg.turb_max_pct, color: "rgba(38,166,201,.12)" }],
    chart_level: () => [{ from: activeCfg.level_min_pct, to: 100, color: "rgba(45,143,213,.12)" }],
};
const bandPlugin = {
    id: "bands",
    beforeDatasetsDraw(chart) {
        const fn = BAND_FN[chart.canvas && chart.canvas.id];
        const y = chart.scales && chart.scales.y, a = chart.chartArea;
        if (!fn || !y || !a) return;
        const ctx = chart.ctx;
        fn().forEach(b => {
            const top = y.getPixelForValue(Math.min(b.to, y.max));
            const bottom = y.getPixelForValue(Math.max(b.from, y.min));
            if (!(bottom > top)) return;
            ctx.save();
            ctx.fillStyle = b.color;
            ctx.fillRect(a.left, top, a.right - a.left, bottom - top);
            ctx.restore();
        });
    },
};

function cssVar(name) { return getComputedStyle(document.documentElement).getPropertyValue(name).trim(); }
function themeCharts() {
    const muted = cssVar("--muted") || "#5b6e80", border = cssVar("--border") || "#d0e0ec", text = cssVar("--text") || "#16222e";
    Object.values(charts).forEach(ch => {
        const o = ch.options;
        o.scales.x.ticks.color = muted; o.scales.y.ticks.color = muted;
        o.scales.x.grid.color = border; o.scales.y.grid.color = border;
        o.plugins.title.color = text;
        ch.update();
    });
}

function initCharts() {
    if (!pluginRegistered) { Chart.register(bandPlugin); pluginRegistered = true; }
    CHART_DEFS.forEach(def => {
        charts[def.id] = new Chart($(def.id), {
            type: "line",
            data: {
                datasets: [{
                    data: [], borderColor: def.color, backgroundColor: def.color,
                    pointRadius: 0, pointHoverRadius: 4, borderWidth: 2, tension: 0.25, spanGaps: false,
                    segment: { borderColor: ctx => segColor(def.key, ctx.p1 && ctx.p1.parsed && ctx.p1.parsed.y) },
                }],
            },
            options: {
                animation: false, responsive: true, maintainAspectRatio: false,
                interaction: { mode: "nearest", axis: "x", intersect: false },
                plugins: {
                    legend: { display: false },
                    title: { display: true, text: def.title },
                    tooltip: { callbacks: { title: items => items.length ? fmtDateTime(items[0].parsed.x) : "" } },
                },
                scales: {
                    x: { type: "linear", ticks: { maxTicksLimit: 6, maxRotation: 0, callback: v => fmtClock(v) }, grid: {} },
                    y: { suggestedMin: def.min, suggestedMax: def.max, ticks: { maxTicksLimit: 6 }, grid: {} },
                },
            },
        });
    });
    themeCharts();
    refreshCharts();
}

function seriesFor(key) {
    const out = [];
    let prev = 0;
    for (const p of history) {
        if (prev && p.t - prev > GAP_MS) out.push({ x: prev + 1, y: null });   // ngắt nét khi mất dữ liệu lâu
        out.push({ x: p.t, y: p[key] });
        prev = p.t;
    }
    return out;
}
function refreshCharts() {
    if (!chartsReady) return;
    CHART_DEFS.forEach(def => {
        const c = charts[def.id];
        c.data.datasets[0].data = seriesFor(def.key);
        c.update();
    });
}

/* ====== XỬ LÝ TIN NHẬN ====== */
function onTelemetry(d, retained) {
    const now = Date.now();
    // Tin sống: coi là mới nhất ngay lúc nhận. Tin retained: chỉ tin vào ts (nếu ts = 0 thì chưa rõ thời điểm).
    const dataTime = !retained ? now : ((typeof d.ts === "number" && d.ts > 0) ? d.ts * 1000 : 0);
    lastTs = dataTime;
    haveData = true;
    lastTele = d;
    if (!retained) offlineFlag = false;        // có telemetry sống = bể đang chạy (mục D8)

    const levels = renderCards(d);
    renderAlert(overallRisk(d, levels), d);

    if (dataTime && dataTime - lastHistT >= HIST_STEP_MS) {
        lastHistT = dataTime;
        history.push({ t: dataTime, temp: num(d.temp_c), ph: num(d.ph), turb: num(d.turb_pct), level: num(d.level_pct) });
        const cut = now - HIST_MAX_MS;
        while (history.length && history[0].t < cut) history.shift();
        refreshCharts();
    }
    tick();
}

function onStatus(s) {
    if (s === "offline") offlineFlag = true;
    else if (s === "online") offlineFlag = false;
    renderTankStatus();
}

function computeOnline() {
    const fresh = lastTs > 0 && (Date.now() - lastTs) <= STALE_MS;
    return fresh && !offlineFlag;
}

function renderTankStatus() {
    const b = $("tankStatus");
    let on;
    if (!haveData) on = offlineFlag ? false : null;
    else on = computeOnline();

    if (on === null) { b.textContent = "Bể: chưa rõ"; b.className = "badge"; return; }
    b.textContent = on ? "Bể: online" : "Bể: offline";
    b.className = "badge " + (on ? "ok" : "bad");

    // Ghi sự kiện khi trạng thái đổi trong lúc trang đang mở (không ghi lần đầu mở trang)
    const bucket = Math.floor(Date.now() / 60000);
    if (prevOnline === true && on === false) {
        wentOffline = true;
        pushEvent({ k: "net|off|" + bucket, t: Date.now(), cls: "ev-off", txt: "🔌 Bể mất kết nối (offline)" });
    } else if (prevOnline === false && on === true && wentOffline) {
        wentOffline = false;
        pushEvent({ k: "net|on|" + bucket, t: Date.now(), cls: "ev-on", txt: "🟢 Bể đã online trở lại" });
    }
    prevOnline = on;
}

function endFeed() {
    pendingFeed = null;
    clearTimeout(feedTimer);
    $("feedBtn").disabled = false;
}

function onEvent(e) {
    if (!e || typeof e !== "object") return;
    const now = Date.now();
    const ts = (typeof e.ts === "number" && e.ts > 0) ? e.ts * 1000 : 0;
    const type = String(e.type || "?");
    let txt, cls, skip = false;

    if (type === "boot") { txt = "🔄 Thiết bị khởi động (firmware " + (e.fw || "?") + ")"; cls = "ev-boot"; }
    else if (type === "feed_done") { txt = "🍤 Đã cho ăn " + (e.portions || 1) + " phần"; cls = "ev-feed"; }
    else if (type === "feed_rejected") { txt = "⛔ Từ chối cho ăn: " + (REASON_TEXT[e.reason] || e.reason || "không rõ lý do"); cls = "ev-reject"; }
    else if (type === "alarm") {
        const lv = clamp(Math.round(Number(e.level)) || 1, 1, 3);
        txt = (lv >= 3 ? "🚨 " : "⚠ ") + "Cảnh báo mức " + lv + ": " + (flagListText(e.msg) || "có chỉ số bất thường");
        cls = "ev-lv" + lv;
        // Bỏ qua nếu cảnh báo cùng mức vừa được ghi chưa đủ giãn cách (events đã sắp mới nhất lên đầu)
        const last = events.find(x => x.cls === cls);
        if (last && (ts || now) - last.t < ALARM_GAP_MS[lv]) skip = true;
    }

    else { txt = "ℹ Sự kiện: " + type; cls = "ev-local"; }

    // Khóa chống trùng: có ts thì dùng ts; không có ts mà có id thì dùng id; còn lại gom theo phút nhận
    const stamp = ts ? String(ts) : (e.id ? "-" : "r" + Math.floor(now / 60000));
    const k = ["ev", type, stamp, e.id || "", e.msg || "", e.level || "", e.reason || ""].join("|");
    if (!skip) pushEvent({ k, t: ts || now, cls, txt });

    if (pendingFeed && e.id === pendingFeed) {
        if (type === "feed_done") { endFeed(); showMsg("feedMsg", "✔ Đã cho ăn xong.", "ok"); }
        else if (type === "feed_rejected") { endFeed(); showMsg("feedMsg", "✖ Bị từ chối: " + (REASON_TEXT[e.reason] || e.reason), "bad"); }
    }
}

function fillConfig(d) {
    if (!d || typeof d !== "object") return;
    const next = Object.assign({}, DEFAULT_CFG);
    for (const k in CFG_FIELDS) {
        if (typeof d[k] === "number" && isFinite(d[k])) { $(CFG_FIELDS[k]).value = d[k]; next[k] = d[k]; }
    }
    if (typeof d.buzzer_on === "boolean") { $("s_buzzer").checked = d.buzzer_on; next.buzzer_on = d.buzzer_on; }
    if (Array.isArray(d.feed_times)) { $("s_times").value = d.feed_times.join(", "); next.feed_times = d.feed_times.slice(); }
    // Cặp ngưỡng sai thứ tự thì dùng mặc định để màu không bị lệch
    if (next.temp_min >= next.temp_max) { next.temp_min = DEFAULT_CFG.temp_min; next.temp_max = DEFAULT_CFG.temp_max; }
    if (next.ph_min >= next.ph_max) { next.ph_min = DEFAULT_CFG.ph_min; next.ph_max = DEFAULT_CFG.ph_max; }
    activeCfg = next;
    if (lastTele) { const lv = renderCards(lastTele); renderAlert(overallRisk(lastTele, lv), lastTele); }
    refreshCharts();
}

/* ====== KẾT NỐI MQTT ====== */
function connect(user, pass) {
    if (client) { try { client.end(true); } catch (e) { } }
    showMsg("loginMsg", "Đang kết nối…", "");
    setConn("Đang kết nối…", "warn");

    const c = mqtt.connect(BROKER, {
        username: user, password: pass,
        clientId: "web-" + Math.random().toString(36).slice(2, 10),   // ngẫu nhiên: mở nhiều tab không đá nhau
        clean: true, keepalive: 30, reconnectPeriod: 3000, connectTimeout: 10000,
    });
    client = c;

    const t = setTimeout(() => {
        if (client === c && !c.connected && !$("login").hidden)
            showMsg("loginMsg", "Chưa kết nối được sau 15 giây. Kiểm tra mạng, cổng 8084 và đường dẫn /mqtt.", "bad");
    }, 15000);

    c.on("connect", () => {
        if (client !== c) return;
        clearTimeout(t);
        setConn("Đã kết nối", "ok");
        showApp();
        c.subscribe(`fishtank/${TANK}/#`, { qos: 1 }, (err) => {
            if (err) setConn("Không subscribe được (kiểm tra ACL)", "bad");
        });
    });
    c.on("reconnect", () => { if (client === c) setConn("Mất kết nối, đang tự nối lại…", "warn"); });
    c.on("close", () => { if (client === c) setConn("Mất kết nối, đang tự nối lại…", "warn"); });
    c.on("error", (err) => {
        if (client !== c) return;
        const m = String((err && err.message) || err);
        const code = err && err.code;
        if (/bad user|not authorized|password/i.test(m) || [4, 5, 134, 135].includes(code)) {
            clearTimeout(t);
            c.end(true); client = null;
            ssDel("ft_u"); ssDel("ft_p");
            showLogin("Sai tên đăng nhập hoặc mật khẩu.");
        } else {
            setConn("Lỗi kết nối: " + m, "bad");
        }
    });
    c.on("message", (topic, payload, packet) => {
        const retained = !!(packet && packet.retain);
        const text = payload.toString();
        if (topic === TOPIC.status) { onStatus(text.trim()); return; }
        let d;
        try { d = JSON.parse(text); } catch (e) { console.warn("JSON lỗi", topic, text); return; }
        if (topic === TOPIC.telemetry) onTelemetry(d, retained);
        else if (topic === TOPIC.event) onEvent(d);
        else if (topic === TOPIC.config) fillConfig(d);
    });
}

/* ====== CHO ĂN ====== */
$("feedBtn").addEventListener("click", () => {
    if (!client || !client.connected) { showMsg("feedMsg", "Chưa kết nối.", "bad"); return; }
    const offline = haveData ? !computeOnline() : false;
    const q = offline
        ? "Bể đang offline hoặc dữ liệu đã cũ, lệnh có thể không tới được thiết bị. Vẫn gửi lệnh cho ăn 1 phần?"
        : "Cho cá ăn 1 phần ngay bây giờ?";
    if (!confirm(q)) return;
    const id = "web-" + Date.now().toString(36) + Math.random().toString(36).slice(2, 5);
    pendingFeed = id;
    $("feedBtn").disabled = true;
    showMsg("feedMsg", "Đang gửi lệnh…", "");
    client.publish(TOPIC.feed, JSON.stringify({ id, portions: 1 }), { qos: 1 }, (err) => {
        if (err) { endFeed(); showMsg("feedMsg", "Gửi lệnh thất bại.", "bad"); }
    });
    feedTimer = setTimeout(() => {
        if (pendingFeed === id) { endFeed(); showMsg("feedMsg", "Chưa nhận được phản hồi từ thiết bị.", "warn"); }
    }, FEED_WAIT_MS);
});

/* ====== CÀI ĐẶT ====== */
function readNum(id) { const s = $(id).value.trim(); return s === "" ? NaN : Number(s); }
function validateConfig(c) {
    for (const k in CFG_FIELDS) if (!Number.isFinite(c[k])) return "Có ô còn trống hoặc không phải số.";
    if (c.temp_min >= c.temp_max) return "Nhiệt độ thấp nhất phải nhỏ hơn cao nhất.";
    if (c.temp_min < LIMIT.tempMin || c.temp_max > LIMIT.tempMax)
        return `Nhiệt độ phải nằm trong biên an toàn ${LIMIT.tempMin}–${LIMIT.tempMax} °C.`;
    if (c.ph_min >= c.ph_max) return "pH thấp nhất phải nhỏ hơn cao nhất.";
    if (c.ph_min < LIMIT.phMin || c.ph_max > LIMIT.phMax)
        return `pH phải nằm trong biên an toàn ${String(LIMIT.phMin).replace(".", ",")}–${String(LIMIT.phMax).replace(".", ",")}.`;
    if (c.turb_max_pct < 0 || c.turb_max_pct > 100) return "Độ đục tối đa phải từ 0 đến 100.";
    if (c.level_danger_pct < 0 || c.level_danger_pct >= c.level_min_pct || c.level_min_pct > 100)
        return "Mực nước: mức nguy hiểm phải nhỏ hơn mức cảnh báo, và cảnh báo không quá 100.";
    if (c.feed_ms < 100 || c.feed_ms > 3000) return "Thời gian quay một phần phải từ 100 đến 3000 ms.";
    if (c.mute_min < 0 || c.mute_min > 240) return "Tắt tiếng phải từ 0 đến 240 phút.";
    if (!c.feed_times.every(t => /^([01]\d|2[0-3]):[0-5]\d$/.test(t))) return "Giờ cho ăn phải dạng HH:MM, ví dụ 08:00.";
    if (new Set(c.feed_times).size !== c.feed_times.length) return "Có giờ cho ăn bị trùng nhau.";
    if (c.feed_times.length > MAX_FEED_TIMES) return `Tối đa ${MAX_FEED_TIMES} mốc giờ cho ăn (chừa 1 lần cho "Cho ăn ngay").`;
    return null;
}
$("saveBtn").addEventListener("click", () => {
    if (!client || !client.connected) { showMsg("saveMsg", "Chưa kết nối.", "bad"); return; }
    const cfg = { v: 1 };
    for (const k in CFG_FIELDS) cfg[k] = readNum(CFG_FIELDS[k]);
    cfg.buzzer_on = $("s_buzzer").checked;
    cfg.feed_times = $("s_times").value.split(",").map(s => s.trim()).filter(Boolean).sort();
    const err = validateConfig(cfg);
    if (err) { showMsg("saveMsg", err, "bad"); return; }
    $("s_times").value = cfg.feed_times.join(", ");
    client.publish(TOPIC.config, JSON.stringify(cfg), { qos: 1, retain: true }, (e) => {
        showMsg("saveMsg", e ? "Lưu thất bại." : "Đã gửi cài đặt. Thiết bị sẽ áp dụng khi nhận được.", e ? "bad" : "ok");
        if (!e) logLocal("⚙ Đã gửi cài đặt mới (nhiệt độ " + cfg.temp_min + "–" + cfg.temp_max + " °C, pH " + cfg.ph_min + "–" + cfg.ph_max + ")", "ev-local");
    });
});

/* ====== ĐĂNG NHẬP / ĐĂNG XUẤT ====== */
$("loginForm").addEventListener("submit", (ev) => {
    ev.preventDefault();
    const u = $("user").value.trim(), p = $("pass").value;
    if (!u || !p) { showMsg("loginMsg", "Hãy nhập tên đăng nhập và mật khẩu.", "bad"); return; }
    ssSet("ft_u", u); ssSet("ft_p", p);       // chỉ giữ trong phiên, đóng tab là mất
    connect(u, p);
});
$("logoutBtn").addEventListener("click", () => {
    if (client) { client.end(true); client = null; }
    ssDel("ft_u"); ssDel("ft_p");
    $("pass").value = "";
    showLogin("");
});
$("clearEvents").addEventListener("click", () => {
    if (!confirm("Xóa lịch sử sự kiện lưu trong trình duyệt này?")) return;
    events = [];
    try { localStorage.removeItem(EVT_KEY); } catch (e) { }
    renderEvents();
});

/* ====== ĐỒNG HỒ: phát hiện dữ liệu cũ, trạng thái bể ====== */
function tick() {
    const u = $("updated");
    if (haveData) {
        const age = lastTs ? Date.now() - lastTs : Infinity;
        const stale = age > STALE_MS;
        let when;
        if (!lastTs) when = "chưa rõ thời điểm (dữ liệu lưu sẵn)";
        else when = age > 12 * 3600 * 1000 ? fmtDateTime(lastTs) : fmtTime(lastTs);
        u.textContent = "Cập nhật: " + when + (stale ? " · dữ liệu cũ" : "");
        u.className = "muted" + (stale ? " old" : "");
        $("cards").classList.toggle("old", stale);
        $("alert").classList.toggle("old", stale);
    }
    renderTankStatus();
}

/* ====== KHỞI ĐỘNG ====== */
$("broker").value = BROKER;
$("tankCode").textContent = TANK;
history = loadHistory();
lastHistT = history.length ? history[history.length - 1].t : 0;
events = loadEvents();
renderEvents();
setInterval(tick, 2000);
setInterval(saveHistory, 60000);
window.addEventListener("pagehide", saveHistory);
document.addEventListener("visibilitychange", () => { if (document.hidden) saveHistory(); else tick(); });
// Mở nhiều tab: tab khác ghi sự kiện mới thì tab này cập nhật theo
window.addEventListener("storage", (ev) => { if (ev.key === EVT_KEY) { events = loadEvents(); renderEvents(); } });
// Đổi giao diện sáng/tối: vẽ lại màu thẻ và biểu đồ
try {
    const mq = window.matchMedia("(prefers-color-scheme: dark)");
    const onScheme = () => {
        if (lastTele) renderCards(lastTele);
        if (chartsReady) themeCharts();
    };
    if (mq.addEventListener) mq.addEventListener("change", onScheme); else if (mq.addListener) mq.addListener(onScheme);
} catch (e) { }
if ("serviceWorker" in navigator) {
    window.addEventListener("load", () => { navigator.serviceWorker.register("sw.js").catch(() => { }); });
}
const u0 = ssGet("ft_u"), p0 = ssGet("ft_p");
if (u0 && p0) { $("user").value = u0; connect(u0, p0); }