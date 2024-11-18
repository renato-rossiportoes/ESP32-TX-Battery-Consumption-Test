#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "_buttons.h"
#include "__config.h"
#include "_rfrx.h"
#include "_lcd_i2c.h"
#include "_storage.h"
#include "_commands.h"

#define DEBOUNCE_TIME_MS 50  // Tempo de debounce em milissegundos
#define DEBOUNCE_TIME_RESET_MS 3000 // Tempo de debounce do botão de reset

static const char *TAG = "BUTTONS";


// Estrutura para armazenar as informações dos botões
typedef struct {
    int gpio_num;
    TimerHandle_t debounce_timer;
    button_event_t event;
} button_t;

// Array de botões, com um para cada GPIO configurado
static button_t buttons[] = {
    {BUTTON1_GPIO, NULL, BUTTON1_PRESSED},
    {BUTTON2_GPIO, NULL, BUTTON2_PRESSED},
    {BUTTON3_GPIO, NULL, BUTTON3_PRESSED}
};

// Fila de eventos para botões
static QueueHandle_t button_event_queue;

// Função chamada quando o timer de debounce expira
static void debounce_timer_callback(TimerHandle_t xTimer) {
    button_t *button = (button_t *) pvTimerGetTimerID(xTimer);

    // Verifica o estado do botão e envia o evento se ele ainda estiver pressionado
    if (gpio_get_level(button->gpio_num) == 0) {  // Assumindo que 0 indica pressionado
        xQueueSend(button_event_queue, &button->event, portMAX_DELAY);
    }
}

// ISR para tratar o pressionamento do botão
static void IRAM_ATTR button_isr_handler(void *arg) {
    button_t *button = (button_t *) arg;

    // Reinicia o timer de debounce
    xTimerStartFromISR(button->debounce_timer, NULL);
}

// Inicializa os GPIOs e configura o debounce dos botões
void buttons_init(QueueHandle_t event_queue) {
    button_event_queue = event_queue;

    for (int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
        button_t *button = &buttons[i];

        // Configuração do GPIO
        gpio_set_direction(button->gpio_num, GPIO_MODE_INPUT);
        gpio_set_pull_mode(button->gpio_num, GPIO_PULLUP_ONLY);
        gpio_set_intr_type(button->gpio_num, GPIO_INTR_NEGEDGE);  // Interrupção na borda de descida

        // Criação do timer de debounce
        if (button->gpio_num == BUTTON1_GPIO) // Botão 1
        {
            button->debounce_timer = xTimerCreate("debounce__onoff_timer", pdMS_TO_TICKS(DEBOUNCE_TIME_MS), pdFALSE, button, debounce_timer_callback);
        }

        if (button->gpio_num == BUTTON2_GPIO) // Botão 2
        {
            button->debounce_timer = xTimerCreate("debounce__reset_timer", pdMS_TO_TICKS(DEBOUNCE_TIME_RESET_MS), pdFALSE, button, debounce_timer_callback);
        }

        if (button->gpio_num == BUTTON3_GPIO) // Botão 3
        {
            button->debounce_timer = xTimerCreate("debounce__button3_timer", pdMS_TO_TICKS(DEBOUNCE_TIME_MS), pdFALSE, button, debounce_timer_callback);
        }

        // Registra o handler de interrupção
        gpio_isr_handler_add(button->gpio_num, button_isr_handler, button);
    }
}



esp_err_t button_register_tx(uint8_t slot){
    set_reg_mode(1);
    printf ("REGISTER - Waiting %dms for a TX...\n", TX_REG_TIMEOUT_MS);
    for (uint16_t r=0; r<TX_REG_TIMEOUT_MS/10; r++){
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if (get_commands_valid_tx()){
            reset_commands_valid_tx();
            register_tx(slot, slot, get_last_buttons_pressed(), get_last_rx_serial()); // Registra último serial validado no slot indicado
            set_reg_mode(0);
            return ESP_OK;
        }
    }
    printf ("Timeout\n");
    reset_commands_valid_tx();
    set_reg_mode(0);
    return ESP_ERR_TIMEOUT;
}




void buttons_task()
{
    QueueHandle_t button_event_queue = xQueueCreate(3, sizeof(button_event_t));
    buttons_init(button_event_queue);
    button_event_t evt;

    while (1)
    {
        // Aguarda um evento de botão
        if (xQueueReceive(button_event_queue, &evt, portMAX_DELAY))
        {
            switch (evt)
            {
            case BUTTON1_PRESSED:
                ESP_LOGI(TAG, "Button 1 pressed");
                set_rele_onoff(!get_rele_onoff()); // Rele on/off
                lcd_refresh_rele_onoff();               
                break;
            case BUTTON2_PRESSED:
                ESP_LOGI(TAG, "Button 2 pressed");
                lcd_set_cursor(0, 1);
                lcd_write_string(" Counter Reset  ");
                lcd_set_cursor(0, 0);
                lcd_write_string(" Counter Reset  ");
                set_rx_rele1_cycles(0);
                set_rx_rele2_cycles(0);
                set_nvs_rx_rele1_cycles(0);
                set_nvs_rx_rele2_cycles(0);
                vTaskDelay(1500 / portTICK_PERIOD_MS);
                lcd_refresh_counter();
                lcd_refresh_rele_onoff();
                break;
            case BUTTON3_PRESSED:
                ESP_LOGI(TAG, "Button 3 pressed");
                printf ("Register TX number 1\n");
                button_register_tx(1);
                printf ("Register TX number 2\n");
                button_register_tx(2);
                break;
            default:
                break;
            }
        }
    }
}