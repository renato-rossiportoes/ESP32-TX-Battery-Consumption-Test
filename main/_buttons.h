#pragma once

#include "freertos/FreeRTOS.h"  // Inclua primeiro para garantir compatibilidade
#include "freertos/queue.h"     // Inclua o queue.h depois

// Definição do tipo de evento de botão
typedef enum {
    BUTTON1_PRESSED,
    BUTTON2_PRESSED,
    BUTTON3_PRESSED,
    BUTTON4_PRESSED,
    TEST_BUTTON1_PRESSED,
    TEST_BUTTON2_PRESSED,
    TEST_BUTTON3_PRESSED,
    TEST_BUTTON4_PRESSED
} button_event_t;


void        buttons_task();
uint8_t     get_motor_first_cycle_flag();
void        set_motor_first_cycle_flag(uint8_t value);