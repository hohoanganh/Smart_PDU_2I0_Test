# Smart PDU 2I0 – Firmware Test

Bộ đóng/ngắt 4 relay chốt (latching relay) dựa trên nền AK MCU KIT
(STM32L151CBT6), không có LCD. Điều khiển relay qua I/O expander **PCA9554A**
trên bus I2C1.

## Phần cứng

| Khối | Kết nối |
|---|---|
| PCA9554A của NXP| I2C1 (PB6/PB7), A0-A2 = GND → addr **0x38**, INT → PB15 |
| Relay chốt 1-4 | P0/P1=RL1, P2/P3=RL2, P4/P5=RL3, P6/P7=RL_BATT (chẵn=ĐÓNG, lẻ=MỞ, xung 200ms rồi nhả) |
| Nút BT1-4 | PB3, PC13, PB4, PB9 — giữ 1s = beep + đảo trạng thái relay tương ứng |
| LED1-4 | PB1, PB12, PB13, PA11 — sáng = relay đang ĐÓNG |
| LED_DBG | PB8 (heartbeat 1Hz) |
| Buzzer | PB0 |
| Console | USART1 PA9/PA10, 115200 |
| RS485 | USART2 PA2/PA3, DIR=PA1 |
| UART3 | PB10/PB11 (Header 2.54 3 pin) |
| SPI Flash | SPI1, CS=PB14 |

Lưu ý relay chốt: trạng thái giữ bằng cơ khí, MCU không đọc lại được — firmware
tự nhớ trạng thái trong RAM (mất điện thì LED/trạng thái về mặc định OFF nhưng
relay vẫn giữ vị trí cũ; dùng `rl all off` để đưa về trạng thái biết trước).

## Build & nạp (PlatformIO)

Mở thư mục project trong VS Code (PlatformIO IDE). Board định nghĩa sẵn trong
`boards/genericSTM32L151CB.json`. Build ✓, nạp → (ST-Link), monitor 🔌 (115200).

## Lệnh CLI

| Lệnh | Chức năng |
|---|---|
| `rl <1-4\|all> <on\|off>` | Đóng/mở relay (vd `rl 2 on`, `rl all off`) |
| `rls` | Trạng thái 4 relay |
| `led` | Toggle LED debug |
| `i2c` | Scan bus I2C (PCA9554=0x38) |
| `flash` / `fwr` | Đọc ID / test ghi-đọc SPI flash |
| `rs485 <text>` | Gửi text qua RS485 |
| `rsl` | Loopback RS485 (nối tắt A-B) |
| `baud rs485 <bps>` | Đổi baudrate RS485 lúc chạy (1200–921600) |
| `ver`, `help` | Phiên bản / danh sách lệnh |

## Nút bấm

BT1→relay 1, BT2→relay 2, BT3→relay 3, BT4→relay 4 — **giữ 1 giây** để đảo
trạng thái relay (beep xác nhận, LED cập nhật). Nhấn/nhả in `BTn: DOWN/UP`
ra console để app test hiển thị trạng thái nút. Chống dội polling 50ms.