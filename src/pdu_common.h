#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

// ===== FIRMWARE VERSION =====
#define FW_VERSION "0.1.0"

// ===== DEVICE ID (van tay de app xac thuc dung thiet bi Smart PDU) =====
#define DEVICE_ID "SMART_PDU_2I0"

// ===== BAUDRATE =====
#define DBG_BAUD   115200
#define RS485_BAUD 9600
#define UART3_BAUD 115200

// ===== LED =====
#define LED_DBG PB8     // LED heartbeat

#define LED1 PB1        // trạng thái relay 1
#define LED2 PB12
#define LED3 PB13
#define LED4 PA11

// LED1-4 ACTIVE-LOW: kích mức THẤP (pin LOW) = LED sáng. LED bám theo trạng
// thái RELAY (giữ liên tục, vì là relay chốt) - không theo xung kích relay.
#define LED_ON_LVL  LOW
#define LED_OFF_LVL HIGH

// ===== BUTTON (toggle relay 1-4) =====
#define BTN1 PB3
#define BTN2 PC13
#define BTN3 PB4
#define BTN4 PB9

// ===== DIP SWITCH 4-bit (INPUT_PULLUP: gat ON/dong = LOW = bit 1) =====
#define DIP1 PA0        // BIT1
#define DIP2 PA4        // BIT2
#define DIP3 PA8        // BIT3
#define DIP4 PA15       // BIT4

// ===== BUZZER =====
// Buzzer ACTIVE HNB09A05 (5V, 3kHz, da co mach dao dong): chi can CAP MUC
// la keu, KHONG can xung tan so (tone). Mac dinh kich muc cao (active-high
#define BUZZER PB0
#define BUZZER_ON_LEVEL  HIGH
#define BUZZER_OFF_LEVEL LOW

// ===== RS485 (USART2) =====
#define RS485_TX  PA2
#define RS485_RX  PA3
#define RS485_DIR PA1

// ===== UART =====
#define UART_DBG_TX PA9
#define UART_DBG_RX PA10

#define UART3_TX PB10
#define UART3_RX PB11

// ===== SPI FLASH =====
#define SPI_SCK  PA5
#define SPI_MISO PA6
#define SPI_MOSI PA7
#define FLASH_CS PB14

// ===== PCA9554 (I2C1 PB6/PB7, A0-A2 = GND -> dia chi 0x38) =====
#define PCA_ADDR 0x38
#define PCA_INT  PB15   // chan INT cua PCA9554

extern HardwareSerial SerialDBG;
extern HardwareSerial SerialRS485;
extern HardwareSerial SerialEXT;
