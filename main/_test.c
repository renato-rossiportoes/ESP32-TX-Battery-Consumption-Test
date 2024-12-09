#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "_test.h"
#include "_buttons.h"
#include "_motor.h"
#include "_storage.h"
#include "__config.h"

static uint8_t test_onoff = 0;
static uint32_t test_button_cycles[4];

void reset_button_cycles(){
    for (uint8_t i = 0; i < NUMBER_OF_TEST_BUTTONS; i++){
        test_button_cycles[i] = 0;
    }
}

void increment_button_cycles(uint8_t button){
    test_button_cycles[button-1]++;
}

void set_button_cycles (uint8_t button, uint32_t cycles){
    test_button_cycles[button-1] = cycles;
}

uint32_t get_button_cycles (uint8_t button){
    return test_button_cycles[button-1];
}

void set_test_onoff(uint8_t value){    
    test_onoff = value;
}

uint8_t get_test_onoff(){
    return test_onoff;
}

void test_task (){
    while(1){
        if (get_test_onoff()){
            if (get_motor_first_cycle_flag() == 1){
                //set_motor_first_cycle_flag(0);
                //motor_first_cycle();
            }

            motor_button_test_cycle(0); // Move a haste para cima
            printf("\n"); // Para dar um espaço entre o log dos clicks dos botões
            vTaskDelay(TEST_BUTTON_REST_MS / portTICK_PERIOD_MS);
            motor_button_test_cycle(1); // Move a haste para baixo
            vTaskDelay(TEST_BUTTON_REST_MS / portTICK_PERIOD_MS);            
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}