#include "pdu_common.h"

// ===== GLOBAL OBJECT =====
HardwareSerial SerialDBG(UART_DBG_RX, UART_DBG_TX);
HardwareSerial SerialRS485(RS485_RX, RS485_TX);
HardwareSerial SerialEXT(UART3_RX, UART3_TX);

#include "hal.h"
#include "pdu_drv.h"

void setup() {

    hal_init();
    drv_init();

    uart_log("SYSTEM INIT");
    uart_log("SMART PDU 2I0 FW " FW_VERSION " (" __DATE__ ")");
    uart_log("Type: help");
}

void loop() {

    cli_process();
    uart3_echo();
    rs485_process();

    btn_process();   // BT1-4 toggle relay 1-4
    dip_process();   // DIP switch 4-bit -> in "DIP: 0xN" khi thay doi
    buzzer_update(); // tat buzzer active dung gio (non-blocking)

    // ===== LED heartbeat =====
    static uint32_t t = 0;
    if (millis() - t > 500) {
        t = millis();
        led_dbg_toggle();
    }
}
