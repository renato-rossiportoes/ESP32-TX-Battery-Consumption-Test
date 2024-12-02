#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "_test.h"
#include "_motor.h"
#include "__config.h"

static uint8_t test_onoff = 0;
static uint64_t button_cycles[4];

void reset_button_cycles(){
    for (uint8_t i = 0; i < NUMBER_OF_TEST_BUTTONS; i++){
        button_cycles[i] = 0;
    }
}

void set_button_cycles (uint8_t button, uint64_t cycles){
    button_cycles[button-1] = cycles;
}

uint64_t get_button_cycles (uint8_t button){
    return button_cycles[button-1];
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
            motor_button_test_cycle(1);
            vTaskDelay(10 / portTICK_PERIOD_MS);
            motor_button_test_cycle(0);
            vTaskDelay(TEST_BUTTON_REST_MS / portTICK_PERIOD_MS);                            
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}