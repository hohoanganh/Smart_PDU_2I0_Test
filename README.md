# Smart PDU 2I0 – Firmware & Test App

Bộ đóng/ngắt **4 relay chốt** (latching relay) dựa trên AK MCU KIT (STM32L151CBT6).  
Điều khiển relay qua I/O expander **PCA9554A** (NXP) trên bus I2C1.

---

## Phần cứng

| Khối                 | Kết nối                                                 | Ghi chú                         |
| -------------------- | ------------------------------------------------------- | ------------------------------- |
| PCA9554A             | I2C1 (PB6/PB7), A0-A2 = GND → addr **0x38**, INT → PB15 |                                 |
| Relay 1 (RL1)        | D2 = OFF, D5 = ON — xung 200 ms                         |                                 |
| Relay 2 (RL2)        | D3 = OFF, D6 = ON — xung 200 ms                         |                                 |
| Relay 3 (RL3)        | D4 = OFF, D7 = ON — xung 200 ms                         |                                 |
| Relay BATT (RL_BATT) | D0 = OFF, D1 = ON — xung 200 ms                         |                                 |
| Nút BT1-4            | PB3, PC13, PB4, PB9                                     | INPUT_PULLUP                    |
| LED1-4               | PB1, PB12, PB13, PA11                                   | Active-low: LOW = sáng          |
| LED_DBG              | PB8                                                     | Heartbeat 1 Hz                  |
| Buzzer               | PB0                                                     | Active-high (HNB09A05 5 V)      |
| Console              | USART1 PA9/PA10                                         | **115200** baud (cố định)       |
| RS485                | USART2 PA2/PA3, DIR = PA1                               | Mặc định 9600, đổi được         |
| UART3                | PB10/PB11                                               | Header 2.54 mm 3-pin            |
| SPI Flash            | SPI1, CS = PB14                                         | Lưu relay state tại addr 0x1000 |
| DIP Switch           | PA0 (BIT1), PA4 (BIT2), PA8 (BIT3), PA15 (BIT4)         | INPUT_PULLUP, ON = LOW          |

> **Relay chốt:** giữ trạng thái bằng cơ khí, không cần cấp điện liên tục.  
> Firmware lưu trạng thái vào **SPI Flash** sau mỗi thay đổi — khôi phục tự động sau reset/mất điện.  
> Nếu cần đặt về trạng thái biết trước: `rl all off`.

---

## Build & Nạp (PlatformIO)

Mở thư mục project trong VS Code + PlatformIO IDE.  
Board: `boards/genericSTM32L151CB.json`.

```
Build  →  pio run
Nạp    →  pio run -t upload   (ST-Link)
Monitor→  pio device monitor --baud 115200
```

---

## Lệnh CLI (Console 115200 baud)

| Lệnh | Chức năng |
|---|---|
| `rl <1-4\|all> <on\|off>` | Đóng/mở relay (vd: `rl 2 on`, `rl all off`) |
| `rls` | Hiển thị trạng thái 4 relay |
| `tled <1-4\|all> <on\|off>` | Điều khiển trực tiếp LED1-4 (độc lập relay) |
| `led` | Toggle LED debug (PB8) |
| `i2c` | Scan bus I2C — PCA9554A sẽ thấy ở 0x38 |
| `flash` | Đọc ID chip SPI Flash |
| `fwr` | Test ghi/đọc SPI Flash |
| `beep <on\|off>` | Bật/tắt tiếng bíp buzzer |
| `dip` | Đọc giá trị DIP Switch 4-bit |
| `rs485 <text>` | Gửi text qua RS485 |
| `rsl` | Loopback RS485 (nối tắt A-B) |
| `baud rs485 <bps>` | Đổi baudrate RS485 lúc chạy (1200–921600) |
| `ver` | Phiên bản firmware |
| `help` | Danh sách lệnh |

---

## Nút bấm

BT1→RL1, BT2→RL2, BT3→RL3, BT4→RL4.  
**Giữ 1 giây** để đảo trạng thái relay (beep xác nhận + LED cập nhật).  
Giữ >3 giây hoặc nhấn nhiều nút đồng thời → 3 beep lỗi, không điều khiển.  
Nhấn/nhả in `BTn: DOWN / UP` ra console. Chống dội polling 50 ms.

---

## App Test (`smart_pdu_test_app.py`)

GUI Python (tkinter) kiểm tra board Smart PDU 2I0 qua cổng COM.

### Cài đặt & chạy

```bash
pip install pyserial openpyxl pillow
python smart_pdu_test_app.py
```

### Build .exe (Windows)

```
build_pdu_exe.bat
```
File exe xuất ra: `dist\Smart_PDU_Test.exe` (không cần Python trên máy đích).

### Chức năng

| Chức năng | Mô tả |
|---|---|
| Kết nối COM | Xác thực ID thiết bị tự động (`SMART_PDU_2I0`) |
| Điều khiển relay | Từng kênh, All ON/OFF, Sync trạng thái xuống thiết bị |
| Giám sát | LED1-4, nút BT1-4, DIP Switch — cập nhật real-time từ serial |
| Test RS485 | Gửi text + kiểm tra loopback, đổi baud ngay trong app |
| Terminal | Gõ lệnh CLI trực tiếp; quick commands (id/ver/i2c/fwr/rls/dip/rsl) |
| Lịch sử lệnh | Phím ↑ / ↓ để duyệt lại lệnh đã gửi (tối đa 50) |
| Run Test | Test sequence tự động: i2c, flash, relay 1–4, LED, button, DIP, RS485 |
| Báo cáo | Xuất kết quả ra `test_report.xlsx` (hoặc CSV nếu không có openpyxl) |

### Lưu ý

- Baud Console cố định: **115200**
- Board **không có RTC** và **không có SHT45** — không có lệnh `rtc`, `time`, `sht`
- RS485 mặc định 9600, đổi được qua lệnh `baud rs485 <n>` hoặc giao diện app
- Sau khi kết nối, app tự đọc trạng thái relay từ thiết bị (`rls`) và đồng bộ giao diện
- Nút **Sync** ghi đè trạng thái relay