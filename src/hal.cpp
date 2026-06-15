#include "hal.h"
#include "pdu_common.h"
#include "pdu_drv.h"

#define CLI_BUF_SIZE 64

static char cmd_buf[CLI_BUF_SIZE];
static uint8_t cmd_idx = 0;

// ===== INIT =====
void hal_init() {

  pinMode(LED_DBG, OUTPUT);
  digitalWrite(LED_DBG, HIGH);

  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  pinMode(BTN4, INPUT_PULLUP);

  pinMode(FLASH_CS, OUTPUT);
  digitalWrite(FLASH_CS, HIGH);

  SerialDBG.begin(DBG_BAUD);
  SerialRS485.begin(RS485_BAUD);
  SerialEXT.begin(UART3_BAUD);

  Wire.begin();
  SPI.begin();
}

// ===== BASIC =====
void led_dbg_toggle() { digitalWrite(LED_DBG, !digitalRead(LED_DBG)); }

void uart_log(const char *msg) { SerialDBG.println(msg); }

// ===== UART3 =====
void uart3_echo() {
  while (SerialEXT.available())
    SerialEXT.write(SerialEXT.read());
}

bool uart3_loopback() {

  while (SerialEXT.available())
    SerialEXT.read();

  const char *pat = "AKTEST";
  SerialEXT.print(pat);
  SerialEXT.flush();

  uint8_t i = 0;
  uint32_t t0 = millis();

  while (millis() - t0 < 100) {
    if (SerialEXT.available()) {
      if ((char)SerialEXT.read() != pat[i])
        return false;
      if (++i == 6)
        return true;
    }
  }
  return false;
}

// ===== BAUDRATE RUNTIME =====
static uint32_t rs485_baud = RS485_BAUD;
static uint32_t u3_baud = UART3_BAUD;

uint32_t rs485_get_baud() { return rs485_baud; }
uint32_t uart3_get_baud() { return u3_baud; }

void rs485_set_baud(uint32_t b) {
  rs485_baud = b;
  SerialRS485.end();
  SerialRS485.begin(b);
  SerialDBG.print("RS485 BAUD: ");
  SerialDBG.println(b);
}

void uart3_set_baud(uint32_t b) {
  u3_baud = b;
  SerialEXT.end();
  SerialEXT.begin(b);
  SerialDBG.print("UART3 BAUD: ");
  SerialDBG.println(b);
}

// ===== I2C =====
void i2c_scan() {
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      SerialDBG.print("I2C: 0x");
      SerialDBG.println(addr, HEX);
    }
  }
}

// ===== FLASH =====
#define CMD_WREN 0x06
#define CMD_RDSR 0x05
#define CMD_PP 0x02
#define CMD_READ 0x03
#define CMD_SE 0x20

static void flash_wren() {
  digitalWrite(FLASH_CS, LOW);
  SPI.transfer(CMD_WREN);
  digitalWrite(FLASH_CS, HIGH);
}

static uint8_t flash_rdsr() {
  digitalWrite(FLASH_CS, LOW);
  SPI.transfer(CMD_RDSR);
  uint8_t s = SPI.transfer(0);
  digitalWrite(FLASH_CS, HIGH);
  return s;
}

static void flash_wait() {
  while (flash_rdsr() & 0x01)
    delay(1);
}

static void flash_send_addr(uint32_t addr) {
  SPI.transfer(addr >> 16);
  SPI.transfer(addr >> 8);
  SPI.transfer(addr);
}

void flash_read_id() {

  digitalWrite(FLASH_CS, LOW);
  SPI.transfer(0x9F);
  uint8_t m = SPI.transfer(0);
  uint8_t t = SPI.transfer(0);
  uint8_t c = SPI.transfer(0);
  digitalWrite(FLASH_CS, HIGH);

  SerialDBG.print("FLASH: ");
  SerialDBG.print(m, HEX);
  SerialDBG.print(" ");
  SerialDBG.print(t, HEX);
  SerialDBG.print(" ");
  SerialDBG.println(c, HEX);
}

static void flash_erase_sector(uint32_t addr) {
  flash_wren();
  digitalWrite(FLASH_CS, LOW);
  SPI.transfer(CMD_SE);
  flash_send_addr(addr);
  digitalWrite(FLASH_CS, HIGH);
  flash_wait();
}

static void flash_write(uint32_t addr, const uint8_t *data, uint16_t len) {
  flash_wren();
  digitalWrite(FLASH_CS, LOW);
  SPI.transfer(CMD_PP);
  flash_send_addr(addr);
  for (uint16_t i = 0; i < len; i++)
    SPI.transfer(data[i]);
  digitalWrite(FLASH_CS, HIGH);
  flash_wait();
}

static void flash_read(uint32_t addr, uint8_t *buf, uint16_t len) {
  digitalWrite(FLASH_CS, LOW);
  SPI.transfer(CMD_READ);
  flash_send_addr(addr);
  for (uint16_t i = 0; i < len; i++)
    buf[i] = SPI.transfer(0);
  digitalWrite(FLASH_CS, HIGH);
}

// Wrapper cong khai de pdu_drv.cpp luu/doc trang thai relay
void flash_read_bytes(uint32_t addr, uint8_t *buf, uint16_t len) {
  flash_read(addr, buf, len);
}
void flash_write_bytes(uint32_t addr, const uint8_t *data, uint16_t len) {
  flash_erase_sector(addr);
  flash_write(addr, data, len);
}
void flash_erase_sector_at(uint32_t addr) { flash_erase_sector(addr); }

void flash_test_rw() {

  SerialDBG.println("FLASH RW TEST");

  uint8_t tx[16], rx[16];
  uint8_t seed = (uint8_t)millis();

  for (uint16_t i = 0; i < 16; i++) {
    tx[i] = i ^ seed;
    rx[i] = 0;
  }

  flash_erase_sector(0);
  flash_write(0, tx, 16);
  flash_read(0, rx, 16);

  SerialDBG.println(memcmp(tx, rx, 16) == 0 ? "FLASH OK" : "FLASH FAIL");
}

// ===== CLI =====
static void cli_execute(char *cmd) {

  // ----- RELAY -----
  if (strncmp(cmd, "rl ", 3) == 0) {

    char ch[8], st[8];

    if (sscanf(cmd, "rl %7s %7s", ch, st) == 2 &&
        (strcmp(st, "on") == 0 || strcmp(st, "off") == 0)) {

      bool on = (strcmp(st, "on") == 0);

      if (strcmp(ch, "all") == 0) {
        for (uint8_t i = 1; i <= 4; i++)
          relay_set(i, on);
        buzzer_beep(2000, 100); // bip xac nhan (giong nhan nut)
        SerialDBG.println("OK");
      } else {
        int n = atoi(ch);
        if (n >= 1 && n <= 4) {
          bool ok = relay_set(n, on);
          if (ok)
            buzzer_beep(2000, 100); // bip xac nhan dao relay
          SerialDBG.println(ok ? "OK" : "PCA FAIL");
        } else
          SerialDBG.println("ERR CH (1-4|all)");
      }
    } else {
      SerialDBG.println("Usage: rl <1-4|all> <on|off>");
    }
  }

  else if (strcmp(cmd, "rls") == 0) {
    relay_print_status();
    SerialDBG.println("OK");
  }

  // ----- ID THIET BI (app xac thuc dung Smart PDU) -----
  else if (strcmp(cmd, "id") == 0) {
    SerialDBG.println("ID: " DEVICE_ID " FW " FW_VERSION);
  }

  // ----- DOC DIP SWITCH 4-bit -----
  else if (strcmp(cmd, "dip") == 0) {
    uint8_t v = dip_read();
    SerialDBG.print("DIP: 0x");
    SerialDBG.println(v, HEX);
  }

  // ----- BAT/TAT tieng bip (test im lang) -----
  else if (strncmp(cmd, "beep", 4) == 0) {
    char st[8];
    if (sscanf(cmd, "beep %7s", st) == 1 &&
        (strcmp(st, "on") == 0 || strcmp(st, "off") == 0)) {
      buzzer_set_mute(strcmp(st, "off") == 0); // "beep off" = tat tieng
      SerialDBG.println(buzzer_is_muted() ? "BEEP OFF" : "BEEP ON");
    } else {
      SerialDBG.println("Usage: beep on|off");
    }
  }

  // ----- LED1-4 (test truc tiep GPIO, doc lap voi relay) -----
  else if (strncmp(cmd, "tled ", 5) == 0) {

    char ch[8], st[8];

    if (sscanf(cmd, "tled %7s %7s", ch, st) == 2 &&
        (strcmp(st, "on") == 0 || strcmp(st, "off") == 0)) {

      bool on = (strcmp(st, "on") == 0);

      if (strcmp(ch, "all") == 0) {
        for (uint8_t i = 1; i <= 4; i++)
          led_set(i, on);
        SerialDBG.println("OK");
      } else {
        int n = atoi(ch);
        if (n >= 1 && n <= 4) {
          led_set(n, on);
          SerialDBG.println("OK");
        } else {
          SerialDBG.println("ERR CH (1-4|all)");
        }
      }
    } else {
      SerialDBG.println("Usage: tled <1-4|all> <on|off>");
    }
  }

  // ----- chung voi AK KIT -----
  else if (strcmp(cmd, "led") == 0) {
    led_dbg_toggle();
    SerialDBG.println("OK");
  }

  else if (strcmp(cmd, "i2c") == 0) {
    i2c_scan();
    SerialDBG.println("OK");
  }

  else if (strcmp(cmd, "flash") == 0) {
    flash_read_id();
    SerialDBG.println("OK");
  }

  else if (strcmp(cmd, "fwr") == 0) {
    flash_test_rw();
  }

  // Alias: "rs485 baud <n>" = doi baud RS485 (giong "baud rs485 <n>")
  else if (strncmp(cmd, "rs485 baud", 10) == 0) {
    unsigned long b = 0;
    if (sscanf(cmd, "rs485 baud %lu", &b) == 1 && b >= 1200 && b <= 921600) {
      rs485_set_baud(b);
      SerialDBG.print("RS485 BAUD: ");
      SerialDBG.println(b);
      SerialDBG.println("OK");
    } else {
      SerialDBG.println("Usage: rs485 baud <1200-921600>");
    }
  }

  else if (strncmp(cmd, "rs485", 5) == 0) {

    char text[64] = "RS485 TEST\n";

    if (sscanf(cmd, "rs485 %63[^\n]", text) == 1)
      rs485_send(text);
    else
      rs485_send("RS485 TEST\n");

    SerialDBG.println("OK");
  }

  else if (strcmp(cmd, "rsl") == 0) {
    SerialDBG.println(rs485_loopback() ? "RS485 LOOP OK" : "RS485 LOOP FAIL");
  }

  else if (strncmp(cmd, "baud", 4) == 0) {

    char port[8];
    unsigned long b = 0;

    if (strcmp(cmd, "baud") == 0) {
      // Truy van: bao baud RS485 hien tai (app hien thi dong)
      SerialDBG.print("RS485 BAUD: ");
      SerialDBG.println(rs485_get_baud());
    } else if (sscanf(cmd, "baud %7s %lu", port, &b) == 2 && b >= 1200 &&
               b <= 921600) {

      if (strcmp(port, "rs485") == 0) {
        rs485_set_baud(b);
        SerialDBG.print("RS485 BAUD: "); // bao baud moi cho app
        SerialDBG.println(b);
        SerialDBG.println("OK");
      } else {
        SerialDBG.println("ERR PORT (rs485)");
      }
    } else {
      SerialDBG.println("Usage: baud rs485 <1200-921600>");
    }
  }

  else if (strcmp(cmd, "ver") == 0) {
    SerialDBG.println("FW " FW_VERSION " (" __DATE__ " " __TIME__ ")");
  }

  else if (strcmp(cmd, "help") == 0) {
    SerialDBG.println("rl <1-4|all> <on|off>");
    SerialDBG.println("rls");
    SerialDBG.println("tled <1-4|all> <on|off>");
    SerialDBG.println("led");
    SerialDBG.println("i2c");
    SerialDBG.println("flash / fwr");
    SerialDBG.println("beep on|off");
    SerialDBG.println("dip");
    SerialDBG.println("rs485 <text>");
    SerialDBG.println("rsl (loopback RS485)");
    SerialDBG.println("baud rs485 <bps>");
    SerialDBG.println("ver");
  }

  else {
    SerialDBG.println("ERR");
  }
}

// ===== CLI PROCESS =====
void cli_process() {

  while (SerialDBG.available()) {

    char c = SerialDBG.read();

    if (c == '\r' || c == '\n') {
      if (cmd_idx > 0) {
        cmd_buf[cmd_idx] = 0;
        cli_execute(cmd_buf);
        cmd_idx = 0;
      }
    } else if (cmd_idx < CLI_BUF_SIZE - 1) {
      cmd_buf[cmd_idx++] = c;
    }
  }
}