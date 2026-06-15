#pragma once
#include <Arduino.h>

void drv_init();

// ===== RELAY CHOT (qua PCA9554) =====
// Moi relay 2 chan dieu khien: P chan(2n)=DONG, P le(2n+1)=MO, xung 200ms
bool relay_set(uint8_t ch, bool on);   // ch = 1..4
bool relay_get(uint8_t ch);
void relay_print_status();
bool pca_ok();

// ===== LED1-4 (dieu khien truc tiep GPIO, doc lap voi relay) =====
bool led_set(uint8_t ch, bool on);     // ch = 1..4

// ===== BUTTON (BT1-4 toggle relay 1-4) =====
void btn_process();

// ===== DIP SWITCH 4-bit =====
uint8_t dip_read();      // gia tri 4-bit: bit0=DIP1(BIT1) ... bit3=DIP4(BIT4)
void dip_process();      // poll + in "DIP: 0xN" khi thay doi (giong nut nhan)

// ===== BUZZER =====
void buzzer_beep(uint16_t freq, uint16_t time);
void buzzer_update();   // goi trong loop() de tat buzzer dung gio (non-blocking)
void buzzer_set_mute(bool m);   // tat/bat tieng bip (test im lang)
bool buzzer_is_muted();

// ===== RS485 =====
void rs485_send(const char *msg);
void rs485_process();
bool rs485_loopback();

// ===== RTC (PCF85063, 0x51) =====
void rtc_test();
bool rtc_get_time(uint8_t *hh, uint8_t *mm, uint8_t *ss);
bool rtc_set_time(uint8_t hh, uint8_t mm, uint8_t ss);

// ===== SHT45 (0x44, neu co gan) =====
bool sht45_read(float *temp, float *hum);
