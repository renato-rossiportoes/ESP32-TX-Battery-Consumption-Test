#pragma once

// Motor de passo
#define STEP_PIN    GPIO_NUM_16
#define DIR_PIN     GPIO_NUM_17

// LCD
#define I2C_MASTER_SCL_IO 22        // GPIO number for I2C master clock
#define I2C_MASTER_SDA_IO 21        // GPIO number for I2C master data

// Pinos dos botões de configuração
#define BUTTON1_GPIO 13
#define BUTTON2_GPIO 12
#define BUTTON3_GPIO 14
#define BUTTON4_GPIO 27

// Pinos dos botões a serem testados
#define TEST_BUTTON_PIN_1 26
#define TEST_BUTTON_PIN_2 25
#define TEST_BUTTON_PIN_3 33
#define TEST_BUTTON_PIN_4 32


#define MOTOR_FREQ 2400  
#define MOTOR_TEST_PULSES 15    // Configurar o driver com [] pulsos para uma volta completa
#define TEST_BUTTON_REST_MS 70

//  Número de botões a serem testados
#define NUMBER_OF_TEST_BUTTONS 4

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"
#define PROMPT_STR CONFIG_IDF_TARGET

// Text Colors
#define TCOLORY        "\033[0;93m"
#define TCOLORB        "\033[0;94m"
#define TCOLOR_RESET    "\033[0m"