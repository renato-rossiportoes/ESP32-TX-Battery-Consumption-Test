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
#include "_lcd_i2c.h"
#include "_test.h"
#include "__config.h"


#define I2C_MASTER_NUM I2C_NUM_0    // I2C port number for master dev
#define I2C_MASTER_FREQ_HZ 100000   // I2C master clock frequency

#define LCD_ADDR 0x3F               // I2C address of the LCD
#define LCD_COLS 16                 // Number of columns in the LCD
#define LCD_ROWS 2                  // Number of rows in the LCD

// Comandos para o LCD
#define LCD_CMD 0x00
#define LCD_DATA 0x40
#define LCD_BACKLIGHT 0x08

static char llu_to_str[20];

void initialize_i2c()
{
 // Configuração do I2C
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };
    i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
    i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);
}


// Envia um byte (comando ou dado) ao LCD
void lcd_write_byte(uint8_t val, uint8_t mode) {
    uint8_t data_h = (val & 0xF0) | mode | 0x08; // High nibble com backlight
    uint8_t data_l = ((val << 4) & 0xF0) | mode | 0x08; // Low nibble com backlight

    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data_h, 1, pdMS_TO_TICKS(10));
    data_h |= 0x04; // Enable bit on
    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data_h, 1, pdMS_TO_TICKS(10));
    data_h &= ~0x04; // Enable bit off
    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data_h, 1, pdMS_TO_TICKS(10));

    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data_l, 1, pdMS_TO_TICKS(10));
    data_l |= 0x04; // Enable bit on
    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data_l, 1, pdMS_TO_TICKS(10));
    data_l &= ~0x04; // Enable bit off
    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data_l, 1, pdMS_TO_TICKS(10));
}

// Função para enviar um comando ao LCD
void lcd_send_cmd(uint8_t cmd) {
    lcd_write_byte(cmd, 0x00); // Modo comando
    vTaskDelay(pdMS_TO_TICKS(10)); // Delay aumentado para estabilidade
}

// Função para enviar dados ao LCD
void lcd_send_data(uint8_t data) {
    lcd_write_byte(data, 0x01); // Modo dados
    vTaskDelay(pdMS_TO_TICKS(10)); // Delay aumentado para estabilidade
}

// Configura o cursor no LCD
void lcd_set_cursor(uint8_t col, uint8_t row) {
    uint8_t cmd = (row == 0) ? (0x80 + col) : (0xC0 + col);
    lcd_send_cmd(cmd);
}

// Escreve uma string no LCD
void lcd_write_string(const char *str) {
    while (*str) {
        lcd_send_data((uint8_t)(*str));
        str++;
    }
}


void lcd_clear(){
    lcd_set_cursor (0,0);
    lcd_write_string ("                ");
    lcd_set_cursor (0,1);
    lcd_write_string ("                ");
}


// Inicializa o LCD
void lcd_init() {
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay inicial para estabilizar

    // Sequência de inicialização em modo 4-bit
    lcd_send_cmd(0x03);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_cmd(0x03);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_cmd(0x03);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_cmd(0x02); // Modo 4-bit

    // Configuração do display
    lcd_send_cmd(0x28); // Interface 4-bit, 2 linhas, 5x8 dots
    lcd_send_cmd(0x08); // Display off
    lcd_send_cmd(0x01); // Limpa display
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd_send_cmd(0x06); // Incrementa cursor
    lcd_send_cmd(0x0C); // Display on, cursor off
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_clear();
}


// Vai para a tela do contador e atualiza os valores
void lcd_refresh_counter(){

    sprintf (llu_to_str, "a%llu            ", get_button_cycles(1)); // Converte para string
    lcd_set_cursor(0, 0);
    lcd_write_string(llu_to_str);       // Imprime no LCD

    sprintf (llu_to_str, "b%llu            ", get_button_cycles(2)); // Converte para string
    lcd_set_cursor(0, 1);
    lcd_write_string(llu_to_str);       // Imprime no LCD

    sprintf (llu_to_str, "c%llu            ", get_button_cycles(3)); // Converte para string
    lcd_set_cursor(8, 0);
    lcd_write_string(llu_to_str);       // Imprime no LCD

    sprintf (llu_to_str, "d%llu            ", get_button_cycles(4)); // Converte para string
    lcd_set_cursor(8, 1);
    lcd_write_string(llu_to_str);       // Imprime no LCD
}



// Atualiza o simbolo de teste on/off
void lcd_refresh_test_onoff()
{
    if (get_test_onoff())
    {
        lcd_set_cursor(15, 1);
        lcd_write_string(">");
    }
    else if (!get_test_onoff())
    {
        lcd_set_cursor(15, 1);
        lcd_write_string("|");
    }
}

void i2c_scanner() {

    printf("Scanning I2C bus...\n");
    for (uint8_t address = 1; address < 127; address++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);

        if (err == ESP_OK) {
            printf("I2C device found at address: 0x%02X\n", address);
        }
    }
    printf("I2C scan complete.\n");
}
