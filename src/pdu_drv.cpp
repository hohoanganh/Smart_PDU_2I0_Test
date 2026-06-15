#include "pdu_common.h"
#include "pdu_drv.h"

#define RTC_ADDR 0x51

// =========================================================
// PCA9554 - I/O expander dieu khien relay chot
// =========================================================
#define PCA_REG_IN  0
#define PCA_REG_OUT 1
#define PCA_REG_CFG 3

#define RELAY_PULSE_MS 800  // xung kich relay chot 0.5s roi nha (ca ON va OFF)

static bool pca_present = false;

static bool pca_write(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(PCA_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static bool pca_init() {
    // tat ca P0-P7 = output, muc 0 (khong xung)
    pca_present = pca_write(PCA_REG_OUT, 0x00) &&
                  pca_write(PCA_REG_CFG, 0x00);
    return pca_present;
}

bool pca_ok() { return pca_present; }

// =========================================================
// RELAY CHOT: ch = 1..4
// P0=DONG1 P1=MO1 | P2=DONG2 P3=MO2 | P4=DONG3 P5=MO3 | P6=DONG4 P7=MO4
// Relay chot giu trang thai bang co khi -> MCU phai tu nho trang thai
// =========================================================
static bool relay_state[5] = {false};
static uint32_t relay_settle_until = 0;  // bo qua doc nut ~200ms sau dong cat relay

static void led_update();

bool relay_set(uint8_t ch, bool on) {

    if (ch < 1 || ch > 4) return false;

    uint8_t pin = (ch - 1) * 2 + (on ? 0 : 1);   // chan = dong, le = mo

    if (!pca_write(PCA_REG_OUT, (uint8_t)(1 << pin))) return false;
    // Giu xung RELAY_PULSE_MS nhung VAN phuc vu buzzer_update -> tieng beep tu
    // tat dung 100ms. Khong dung delay() vi se chan, lam nhieu beep dinh thanh
    // 1 tieng dai khi sync/all on-off nhieu relay lien tiep.
    uint32_t _t0 = millis();
    while (millis() - _t0 < RELAY_PULSE_MS) buzzer_update();
    if (!pca_write(PCA_REG_OUT, 0x00)) return false;   // nha xung

    relay_state[ch] = on;
    led_update();

    SerialDBG.print("RL");
    SerialDBG.print(ch);
    SerialDBG.println(on ? ": ON" : ": OFF");
    relay_settle_until = millis() + 200;   // chong doc nut nham do nhieu dong cat
    return true;
}

bool relay_get(uint8_t ch) {
    return (ch >= 1 && ch <= 4) ? relay_state[ch] : false;
}

void relay_print_status() {
    for (uint8_t i = 1; i <= 4; i++) {
        SerialDBG.print("RL");
        SerialDBG.print(i);
        SerialDBG.println(relay_state[i] ? ": ON" : ": OFF");
    }
    if (!pca_present) SerialDBG.println("PCA9554: NOT FOUND");
}

// =========================================================
// LED trang thai relay (LED1-4 sang = relay dang DONG)
// =========================================================
static const uint32_t led_pins[5] = {0, LED1, LED2, LED3, LED4};

static void led_update() {
    // LED bam theo trang thai relay (giu lien tuc). Active-low: ON = LOW.
    for (uint8_t i = 1; i <= 4; i++)
        digitalWrite(led_pins[i], relay_state[i] ? LED_ON_LVL : LED_OFF_LVL);
}

// Dieu khien truc tiep LED1-4 (test, doc lap voi relay).
// Luu y: goi relay_set() se cap nhat lai LED theo trang thai relay.
bool led_set(uint8_t ch, bool on) {

    if (ch < 1 || ch > 4) return false;

    digitalWrite(led_pins[ch], on ? LED_ON_LVL : LED_OFF_LVL);

    SerialDBG.print("LED");
    SerialDBG.print(ch);
    SerialDBG.println(on ? ": ON" : ": OFF");
    return true;
}

// =========================================================
// BUZZER ACTIVE (HNB09A05) - chi cap MUC, khong dung tone()
// =========================================================
static uint32_t buzzer_off_at = 0;   // thoi diem tat buzzer (non-blocking)
static bool     buzzer_muted  = false;  // tat tieng bip (test im lang)

void buzzer_set_mute(bool m) { buzzer_muted = m; }
bool buzzer_is_muted()       { return buzzer_muted; }

// Keu 'time' ms (tham so freq giu lai cho tuong thich, KHONG dung voi buzzer
// active). Non-blocking: buzzer_update() trong loop() se tat dung gio.
void buzzer_beep(uint16_t freq, uint16_t time) {
    (void)freq;
    if (buzzer_muted) return;            // dang tat tieng -> khong keu
    digitalWrite(BUZZER, BUZZER_ON_LEVEL);
    buzzer_off_at = millis() + time;
    if (buzzer_off_at == 0) buzzer_off_at = 1;   // tranh trung gia tri 'tat'
}

void buzzer_update() {
    if (buzzer_off_at && (int32_t)(millis() - buzzer_off_at) >= 0) {
        digitalWrite(BUZZER, BUZZER_OFF_LEVEL);
        buzzer_off_at = 0;
    }
}

// 3 tieng bit ngan bao loi (blocking ~900ms, chi dung cho loi)
static void buzzer_error() {
    if (buzzer_muted) return;            // dang tat tieng -> khong keu
    for (uint8_t i = 0; i < 3; i++) {
        digitalWrite(BUZZER, BUZZER_ON_LEVEL);
        delay(150);
        digitalWrite(BUZZER, BUZZER_OFF_LEVEL);
        delay(150);
    }
    buzzer_off_at = 0;
}

// =========================================================
// BUTTON: BT1-4 (polling)
// - Nhan xuong:           in "BTn: DOWN"
// - Giu [1s,3s) roi NHA:  beep + dao relay n (in "RLn: ON/OFF")
// - Giu > 3s (nut ket):   3 tieng loi + "BT_ERR: HOLD BTn" -> KHONG dao relay
// - 2+ nut cung luc:      3 tieng loi + "BT_ERR: MULTI"     -> KHONG dao relay
// - Nha nut:              in "BTn: UP"
// (Quyet dinh dao relay o luc NHA nut: giu qua lau/nhieu nut se khong dieu khien)
// =========================================================
#define BTN_HOLD_MS     1000   // giu toi thieu de kich relay
#define BTN_MAX_HOLD_MS 3000   // giu qua lau = loi (nut ket)
#define BTN_DEBOUNCE_MS 50     // chong doi

static const uint32_t btn_pins[5] = {0, BTN1, BTN2, BTN3, BTN4};

static void buttons_init() {
    // chan da cau hinh INPUT_PULLUP trong hal_init(), khong dung ngat
}

static void btn_print(uint8_t b, const char *st) {
    SerialDBG.print("BT");
    SerialDBG.print(b);
    SerialDBG.print(": ");
    SerialDBG.println(st);
}

void btn_process() {

    static bool     pressed[5]     = {false};
    static bool     err_beeped[5]  = {false};  // da bao loi -> khoa khong dao relay
    static bool     hold_beeped[5] = {false};  // da bip bao "du 1s, co the nha nut"
    static uint32_t t_down[5]      = {0};
    static uint32_t t_edge[5]      = {0};

    uint32_t now = millis();

    // Ngay sau khi dong/cat relay (app hoac nut): nhieu dien co the lam doc
    // nham chan nut -> keu bip nham. Bo qua quet nut trong cua so nay.
    if ((int32_t)(now - relay_settle_until) < 0) return;

    // Dem so nut dang nhan (phat hien da nut)
    uint8_t n_pressed = 0;
    for (uint8_t i = 1; i <= 4; i++) if (pressed[i]) n_pressed++;

    for (uint8_t b = 1; b <= 4; b++) {

        bool down = (digitalRead(btn_pins[b]) == LOW);

        if (down && !pressed[b]) {                         // vua nhan xuong
            if (now - t_edge[b] < BTN_DEBOUNCE_MS) continue;
            pressed[b]     = true;
            err_beeped[b]  = false;
            hold_beeped[b] = false;
            t_down[b]      = now;
            t_edge[b]      = now;
            btn_print(b, "DOWN");
        }
        else if (down && pressed[b]) {

            uint32_t held = now - t_down[b];

            // Khong dao relay luc dang GIU. Quyet dinh khi NHA nut (ben duoi).
            // Trong luc giu chi phat hien LOI de khoa (err_beeped[b] = true):

            // Nhan NHIEU nut cung luc -> loi, khoa tat ca (chong nham)
            if (n_pressed >= 2 && !err_beeped[b]) {
                bool first = true;
                for (uint8_t i = 1; i <= 4; i++) {
                    if (pressed[i] && !err_beeped[i]) {
                        err_beeped[i] = true;          // khoa: nha nut KHONG dao relay
                        if (first) {
                            first = false;
                            buzzer_error();
                            SerialDBG.println("BT_ERR: MULTI");
                        }
                    }
                }
            }
            // Giu QUA LAU (> 3s) = nut ket -> bao loi 1 lan, khoa khong dao relay
            else if (held >= BTN_MAX_HOLD_MS && !err_beeped[b]) {
                err_beeped[b] = true;                  // khoa: nha nut KHONG dao relay
                buzzer_error();
                SerialDBG.print("BT_ERR: HOLD BT");
                SerialDBG.println(b);
            }
            // Du thoi gian hop le (>= 1s, chua qua 3s, 1 nut) -> BIP 1 tieng bao
            // nguoi dung NHA nut ra (relay se dao khi nha).
            else if (held >= BTN_HOLD_MS && !err_beeped[b] && !hold_beeped[b]) {
                hold_beeped[b] = true;
                buzzer_beep(2000, 100);
            }
        }
        else if (!down && pressed[b]) {                    // nha nut
            if (now - t_edge[b] < BTN_DEBOUNCE_MS) continue;
            uint32_t held = now - t_down[b];
            pressed[b]    = false;
            t_edge[b]     = now;
            // CHI dao relay khi: khong bi khoa loi VA giu trong [1s, 3s).
            // Giu > 3s (nut ket) hoac nhan nhieu nut -> err_beeped -> KHONG dao.
            if (!err_beeped[b] && held >= BTN_HOLD_MS && held < BTN_MAX_HOLD_MS) {
                relay_set(b, !relay_get(b));   // da bip bao luc du 1s o tren
            }
            err_beeped[b]  = false;
            hold_beeped[b] = false;
            btn_print(b, "UP");
        }
    }
}

// =========================================================
// DIP SWITCH 4-bit (PA0,PA4,PA8,PA15) - INPUT_PULLUP: dong/ON = LOW = bit 1
// =========================================================
static const uint32_t dip_pins[4] = {DIP1, DIP2, DIP3, DIP4};

static void dip_init() {
    for (uint8_t i = 0; i < 4; i++) pinMode(dip_pins[i], INPUT_PULLUP);
}

uint8_t dip_read() {
    uint8_t v = 0;
    for (uint8_t i = 0; i < 4; i++)
        if (digitalRead(dip_pins[i]) == LOW) v |= (uint8_t)(1 << i);  // ON = 1
    return v;
}

// Poll DIP, in "DIP: 0xN" khi thay doi (de app cap nhat live nhu nut nhan)
void dip_process() {
    static uint8_t  last  = 0;
    static bool     first = true;
    static uint32_t t_chg = 0;
    static uint8_t  cand  = 0;

    uint8_t v = dip_read();
    uint32_t now = millis();

    if (v != cand) { cand = v; t_chg = now; }      // bat dau chong doi
    if ((first || v != last) && (now - t_chg >= 30)) {  // on dinh 30ms
        first = false;
        last  = v;
        SerialDBG.print("DIP: 0x");
        SerialDBG.println(v, HEX);
    }
}

// =========================================================
// RS485
// =========================================================
void rs485_send(const char *msg) {

    digitalWrite(RS485_DIR, HIGH);
    delayMicroseconds(20);

    SerialRS485.print(msg);
    SerialRS485.flush();

    delayMicroseconds(20);
    digitalWrite(RS485_DIR, LOW);
}

void rs485_process() {

    while (SerialRS485.available())
        SerialDBG.write(SerialRS485.read());   // forward ra console
}

bool rs485_loopback() {

    while (SerialRS485.available()) SerialRS485.read();

    const char *pat = "AKTEST";
    rs485_send(pat);

    uint8_t  i  = 0;
    uint32_t t0 = millis();

    while (millis() - t0 < 200) {
        if (SerialRS485.available()) {
            if ((char)SerialRS485.read() != pat[i]) return false;
            if (++i == 6) return true;
        }
    }
    return false;
}

// =========================================================
// RTC PCF85063
// =========================================================
static uint8_t bcd2dec(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
static uint8_t dec2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

bool rtc_get_time(uint8_t *hh, uint8_t *mm, uint8_t *ss) {

    Wire.beginTransmission(RTC_ADDR);
    Wire.write(0x04);
    if (Wire.endTransmission() != 0) return false;

    if (Wire.requestFrom(RTC_ADDR, 3) != 3) return false;

    *ss = bcd2dec(Wire.read() & 0x7F);
    *mm = bcd2dec(Wire.read() & 0x7F);
    *hh = bcd2dec(Wire.read() & 0x3F);
    return true;
}

bool rtc_set_time(uint8_t hh, uint8_t mm, uint8_t ss) {

    Wire.beginTransmission(RTC_ADDR);
    Wire.write(0x04);
    Wire.write(dec2bcd(ss));
    Wire.write(dec2bcd(mm));
    Wire.write(dec2bcd(hh));
    return Wire.endTransmission() == 0;
}

void rtc_test() {

    uint8_t hh, mm, ss;
    char buf[16];

    if (rtc_get_time(&hh, &mm, &ss))
        snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hh, mm, ss);
    else
        strcpy(buf, "--:--:--");

    SerialDBG.print("RTC: ");
    SerialDBG.println(buf);
}

// =========================================================
// SHT45 (neu co gan tren bus I2C)
// =========================================================
static uint8_t sht45_crc8(const uint8_t *d, uint8_t len) {
    uint8_t crc = 0xFF;
    while (len--) {
        crc ^= *d++;
        for (uint8_t i = 0; i < 8; i++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
    return crc;
}

bool sht45_read(float *temp, float *hum) {

    Wire.beginTransmission(0x44);
    Wire.write(0xFD);
    if (Wire.endTransmission() != 0) return false;

    delay(10);

    if (Wire.requestFrom(0x44, 6) != 6) return false;

    uint8_t d[6];
    for (uint8_t i = 0; i < 6; i++) d[i] = Wire.read();

    if (sht45_crc8(d, 2) != d[2] || sht45_crc8(d + 3, 2) != d[5])
        return false;

    uint16_t t_raw = ((uint16_t)d[0] << 8) | d[1];
    uint16_t h_raw = ((uint16_t)d[3] << 8) | d[4];

    *temp = -45.0f + 175.0f * t_raw / 65535.0f;
    *hum  = -6.0f  + 125.0f * h_raw / 65535.0f;

    if (*hum < 0)   *hum = 0;
    if (*hum > 100) *hum = 100;
    return true;
}

// =========================================================
// INIT
// =========================================================
void drv_init() {

    pinMode(RS485_DIR, OUTPUT);
    digitalWrite(RS485_DIR, LOW);

    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, BUZZER_OFF_LEVEL);   // tat buzzer luc khoi dong

    for (uint8_t i = 1; i <= 4; i++) {
        pinMode(led_pins[i], OUTPUT);
        digitalWrite(led_pins[i], LED_OFF_LVL);   // active-low: tat = HIGH
    }

    pinMode(PCA_INT, INPUT_PULLUP);
    buttons_init();
    dip_init();

    SerialDBG.println(pca_init() ? "PCA9554 OK" : "PCA9554 NOT FOUND");
}
