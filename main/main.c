#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "soc/soc_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"



// App .h
#include "_storage.h"
#include "_console.h"
#include "_lcd_i2c.h"
#include "_buttons.h"
#include "_motor.h"
#include "_test.h"
#include "__config.h"


static const char *TAG = "main";

#define LED_BUILT_IN_GPIO 2


void initialize_onboard_led()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED_BUILT_IN_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}


void app_main(void)
{
    initialize_onboard_led();
    
    initialize_nvs();
    initialize_filesystem();

    initialize_i2c(); 
    i2c_scanner();
    lcd_init();

    initialize_namespace();

    gpio_install_isr_service(0);

    initialize_motor();

    button_cycles_nvs_to_ram();

    lcd_refresh_counter();
    lcd_refresh_test_onoff();

    vTaskDelay(100 / portTICK_PERIOD_MS);

    xTaskCreate(&console_task, "console_task", 4096, NULL, 5, NULL);
    xTaskCreate(&buttons_task, "buttons_task", 8000, NULL, 5, NULL);
    xTaskCreate(&test_task, "test_task", 8000, NULL, 5, NULL);

    ESP_LOGI (TAG, "Creation of tasks done!");      

    while (1)
    {
        // Blink onboard LED
        gpio_set_level(LED_BUILT_IN_GPIO, 1);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        gpio_set_level(LED_BUILT_IN_GPIO, 0);
        vTaskDelay(990 / portTICK_PERIOD_MS);
    }
}