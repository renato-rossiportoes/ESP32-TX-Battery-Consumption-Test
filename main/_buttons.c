#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "driver/gptimer.h"

#include "_buttons.h"
#include "__config.h"
#include "_lcd_i2c.h"
#include "_storage.h"
#include "_test.h"
#include "_commands.h"
#include "_motor.h"

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
    {BUTTON3_GPIO, NULL, BUTTON3_PRESSED},
    {BUTTON4_GPIO, NULL, BUTTON4_PRESSED}
};

// Fila de eventos para botões
static QueueHandle_t button_event_queue;

// Função chamada quando o timer de debounce expira
static void debounce_timer_callback(TimerHandle_t xTimer) {
    button_t *button = (button_t *) pvTimerGetTimerID(xTimer);

    // Verifica o estado do botão e envia o evento se ele ainda estiver pressionado
    if (gpio_get_level(button->gpio_num) == 0) { 
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
            button->debounce_timer = xTimerCreate("debounce__button1_timer", pdMS_TO_TICKS(DEBOUNCE_TIME_MS), pdFALSE, button, debounce_timer_callback);
        }

        if (button->gpio_num == BUTTON2_GPIO) // Botão 2
        {
            button->debounce_timer = xTimerCreate("debounce__button2_timer", pdMS_TO_TICKS(DEBOUNCE_TIME_MS), pdFALSE, button, debounce_timer_callback);
        }

        if (button->gpio_num == BUTTON3_GPIO) // Botão 3
        {
            button->debounce_timer = xTimerCreate("debounce__button3_timer", pdMS_TO_TICKS(DEBOUNCE_TIME_MS), pdFALSE, button, debounce_timer_callback);
        }

        if (button->gpio_num == BUTTON4_GPIO) // Botão 2
        {
            button->debounce_timer = xTimerCreate("debounce__button4_timer", pdMS_TO_TICKS(DEBOUNCE_TIME_RESET_MS), pdFALSE, button, debounce_timer_callback);
        }

        // Registra o handler de interrupção
        gpio_isr_handler_add(button->gpio_num, button_isr_handler, button);
    }
}

void buttons_task()
{
    QueueHandle_t button_event_queue = xQueueCreate(4, sizeof(button_event_t));
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
                set_test_onoff(!get_test_onoff()); // Test on/off'
                if (get_test_onoff())
                {
                    printf("Teste ON\n");
                }
                else if (!get_test_onoff())
                {
                    printf("Teste OFF\n");
                }
                lcd_refresh_test_onoff();
                break;
            case BUTTON2_PRESSED:
                ESP_LOGI(TAG, "Button 2 pressed");
                printf("1 Step CW\n");
                gpio_set_level(DIR_PIN, 0);
                gpio_set_level(STEP_PIN, 0);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(STEP_PIN, 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(STEP_PIN, 0);
                // set_test_onoff(1);
                // set_test_onoff(0);
                break; 
            case BUTTON3_PRESSED:
                ESP_LOGI(TAG, "Button 3 pressed");
                printf("1 Step CCW\n");
                gpio_set_level(DIR_PIN, 1);
                gpio_set_level(STEP_PIN, 0);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(STEP_PIN, 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(STEP_PIN, 0);
               
                break;
            case BUTTON4_PRESSED:
                ESP_LOGI(TAG, "Button 4 pressed");
                lcd_set_cursor(0, 1);
                lcd_write_string(" Counter Reset  ");
                lcd_set_cursor(0, 0);
                lcd_write_string(" Counter Reset  ");
                reset_button_cycles();
                reset_nvs_button_cycles();
                vTaskDelay(1500 / portTICK_PERIOD_MS);
                lcd_refresh_counter();
                lcd_refresh_test_onoff();
            break;                    
            default:
                break;
            }
        }
    }
}