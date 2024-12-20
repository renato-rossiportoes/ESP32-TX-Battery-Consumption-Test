#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "__config.h"

gptimer_handle_t timer;

volatile int pulse_count = 0;
static bool step_state = false;

void motor_timer_start(){
    gptimer_start(timer);    
}

void motor_timer_stop(){
    gptimer_stop(timer);    
}

void motor_start(){
    //gptimer_enable(timer);
    gptimer_start(timer);
}

void motor_stop(){
    gptimer_stop(timer);
    //gptimer_disable(timer);
}

void motor_button_test_cycle(uint8_t direction){
    gpio_set_level(DIR_PIN, direction);
    pulse_count = 0;
    step_state = 0;
    gpio_set_level(STEP_PIN, 0); // Garante estado inicial baixo

    if (direction == 1)
    {
        gptimer_start(timer);
        while (pulse_count < (MOTOR_TEST_PULSES))
        {
            vTaskDelay(10 / portTICK_PERIOD_MS); // Evita busy-waiting
        }
        // Anda um pouco mais pra baixo
        vTaskDelay(50 / portTICK_PERIOD_MS);
        for (uint16_t pulses = 0; pulses < 5; pulses++)
        {
            gpio_set_level(STEP_PIN, 1);
            vTaskDelay(5 / portTICK_PERIOD_MS);
            gpio_set_level(STEP_PIN, 0);
            vTaskDelay(5 / portTICK_PERIOD_MS);
        }
    }

    if (direction == 0)
    {
        gptimer_start(timer);
        while (pulse_count < MOTOR_TEST_PULSES )
        {
            vTaskDelay(10 / portTICK_PERIOD_MS); // Evita busy-waiting
        }
    }
}


static bool IRAM_ATTR timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    //static bool step_state = false;
    gpio_set_level(STEP_PIN, step_state);
    step_state = !step_state;
/*
    if (step_state == 0) {
        pulse_count++;
    }

    // Verifica se atingiu o número de pulsos desejado
    if (pulse_count >= MOTOR_TEST_PULSES) {
        gptimer_stop(timer);  // Para o timer
        return true;          // Retorna true para desativar a interrupção
    }
*/
    return true;
}


void initialize_timer_motor(){
     // Configure GPTimer
    
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 MHz (1 us por tick)
    };
    gptimer_new_timer(&timer_config, &timer);

    gptimer_event_callbacks_t callbacks = {
        .on_alarm = timer_callback,
    };
    gptimer_register_event_callbacks(timer, &callbacks, NULL);

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = timer_config.resolution_hz / MOTOR_FREQ / 2,
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_alarm_action(timer, &alarm_config);

    // Inicia o timer
    pulse_count = 0;  // Reseta o contador
    step_state = 0; // Inicializa o estado
    gpio_set_level(STEP_PIN, 0); // Garante estado inicial baixo

    gptimer_enable(timer);
    //gptimer_start(timer);
}


void initialize_motor()
{
    // Configure GPIOs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << STEP_PIN) | (1ULL << DIR_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
    gpio_set_level(DIR_PIN, 0); // Inicialmente, direção para frente

    initialize_timer_motor();
}


