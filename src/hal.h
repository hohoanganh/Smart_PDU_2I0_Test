#pragma once
#include <Arduino.h>

void hal_init();

void led_dbg_toggle();
void uart_log(const char *msg);

void uart3_echo();
bool uart3_loopback();

// ===== BAUDRATE RUNTIME =====
uint32_t rs485_get_baud();
uint32_t uart3_get_baud();
void rs485_set_baud(uint32_t b);
void uart3_set_baud(uint32_t b);

void i2c_scan();

void flash_read_id();
void flash_test_rw();

void cli_process();
