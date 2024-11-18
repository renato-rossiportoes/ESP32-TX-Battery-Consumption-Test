#pragma once

#include "freertos/FreeRTOS.h"  // Inclua primeiro para garantir compatibilidade
#include "freertos/queue.h"     // Inclua o queue.h depois

// Definição do tipo de evento de botão
typedef enum {
    BUTTON1_PRESSED,
    BUTTON2_PRESSED,
    BUTTON3_PRESSED
} button_event_t;


void buttons_task();