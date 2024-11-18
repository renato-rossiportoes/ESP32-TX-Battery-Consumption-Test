#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "string.h"

#include "_commands.h"
#include "_storage.h"
#include "_lcd_i2c.h"
#include "__config.h"

static const char* TAG = "rfrx";

static QueueHandle_t      log_evt_queue       = NULL;
static gptimer_handle_t   rf_rx_timer = NULL;
static char log_msg[50];
static bool    decoder_66_bit_status  = 0;
static uint8_t decoder_66_bit_position = 0;
static uint8_t decoder_66_bit_values[DECODER_BITS] = {0};
static uint64_t last_edge_time = 0;
static uint64_t current_time = 0;
static uint64_t pulse_duration = 0;
static uint8_t decoder_state = DECODER_S_PREAMBLE;
static uint8_t decoder_66bit_finish = 0;
static uint8_t  preamble_pulse_count = 0;
static char last_rx_serial[29];
static uint8_t last_buttons_pressed = 0;
static uint8_t verbose_mode = 0;
static uint64_t rx_rele1_cycles = 0;
static uint64_t rx_rele2_cycles = 0;
static uint8_t rele_on = 0;


void set_rele_onoff(uint8_t value){
    rele_on = value;
}

uint8_t get_rele_onoff(){
    return rele_on;
}


void set_rx_rele1_cycles(uint64_t value){
    rx_rele1_cycles = value;
}

void set_rx_rele2_cycles(uint64_t value){
    rx_rele2_cycles = value;
}

uint64_t get_rx_rele1_cycles(){
    return rx_rele1_cycles;
}

uint64_t get_rx_rele2_cycles(){
    return rx_rele2_cycles;
}


void create_rfrx_queues()
{
    //rf_rx_evt_queue = xQueueCreate(50, sizeof(gpio_num));
    log_evt_queue = xQueueCreate(50, sizeof(log_msg));
}

void initialize_rf_rx_timer(){
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &rf_rx_timer));
    ESP_ERROR_CHECK(gptimer_enable(rf_rx_timer));
    ESP_ERROR_CHECK(gptimer_start(rf_rx_timer));
}

void set_last_buttons_pressed()
{
    // Guarda os últimos botões pressionados no buffer
    last_buttons_pressed = 0; // Reseta para não sofrer interferência dos valores anteriores
    last_buttons_pressed |= (decoder_66_bit_values[63] & 0x01) << 3; // Bit mais significativo
    last_buttons_pressed |= (decoder_66_bit_values[62] & 0x01) << 2;
    last_buttons_pressed |= (decoder_66_bit_values[61] & 0x01) << 1;
    last_buttons_pressed |= (decoder_66_bit_values[60] & 0x01) << 0; // Bit menos significativo
}

uint8_t get_last_buttons_pressed (){
    return last_buttons_pressed;
}

char *get_last_rx_serial(){
    return last_rx_serial;
}


// Verifica o match do TX (serial e botão pressionado) e executa sua função
void tx_match_execute()
{ 
    nvs_handle_t handle;
    char *last_serial = get_last_rx_serial();
    size_t len = 35;
    char value[len];    
    char key[7];
    uint8_t dont_match = 0;
    char last_button_str[5];
    snprintf (last_button_str, sizeof(last_button_str), "%01d", last_buttons_pressed); // Converte last_buttons_pressed para str para poder comparar com value
    nvs_open("tx", NVS_READWRITE, &handle);
    
    for (uint8_t r = 1; r <= TX_SLOTS_NUM; r++)
    {
        dont_match = 0;
        snprintf (key, sizeof(key), "tx%02d", (uint8_t)r);        
        nvs_get_str(handle, key, value, &len); // Pega o serial armazenado

        // Serial check
        for (signed int t=0; t < 28; t++)
        {
            if (value[t+6] != last_serial[t]){
                dont_match = 1;
                break;
            }            
        }
        // Button check
        if (value[5] != last_button_str[0]){
            dont_match = 1;            
        }

        if (!dont_match){
            printf (TCOLORY "Match key %s\n" TCOLOR_RESET, key);

            // RELE 1 - Incrementa a variável que conta as batidas de rele, grava na nvs e imprime o valor
            if (strcmp(key, "tx01") == 0)
            {
                rx_rele1_cycles++;
                if (rx_rele1_cycles % 10 == 0) set_nvs_rx_rele1_cycles(rx_rele1_cycles); // Só grava de 10 em 10
                printf(TCOLORB "RELE 1 Cycles completed: %llu\n" TCOLOR_RESET, rx_rele1_cycles);
            }

            // RELE 2 - Incrementa a variável que conta as batidas de rele, grava na nvs e imprime o valor
            if (strcmp(key, "tx02") == 0)
            {
                rx_rele2_cycles++;
                if (rx_rele2_cycles % 10 == 0) set_nvs_rx_rele2_cycles(rx_rele2_cycles); // Só grava de 10 em 10
                printf(TCOLORB "RELE 2 Cycles completed: %llu\n" TCOLOR_RESET, rx_rele2_cycles);
            }
            lcd_refresh_counter(); // Atualiza os contadores no LCD
            lcd_refresh_rele_onoff();
        }
    }
    nvs_close(handle);
}




void set_last_rx_serial(uint8_t serial_buffer[28])
{
    for (int r = 0; r < 28; r++) // seta o last_rx_serial convertendo uint8_t em char
    {
        if (serial_buffer[r] == 0)
        {
            last_rx_serial[r] = '0';
        }
        if (serial_buffer[r] == 1)
        {
            last_rx_serial[r] = '1';
        }
        if ((serial_buffer[r] != 0) && (serial_buffer[r] != 1))
        {
            printf("Invalid serial \n");
            return;
        }
    }
    last_rx_serial[29] = 0; // Caractere nulo no fim da string
}






void print_decoded_data(){
    // Imprime os dados recebidos
            printf("* 66 bits: ");
            for (uint8_t r = 0; r < DECODER_BITS; r++)
            {
                printf("%d", decoder_66_bit_values[r]);
            }
            printf("\n");
            if (verbose_mode >= 2)
            {
                printf("[65] Repeat: %d\n", decoder_66_bit_values[65]);
                printf("[64] Low Battery: %d\n", decoder_66_bit_values[64]);
                printf("[63] Button 1: %d\n", decoder_66_bit_values[63]);
                printf("[62] Button 2: %d\n", decoder_66_bit_values[62]);
                printf("[61] Button 3: %d\n", decoder_66_bit_values[61]);
                printf("[60] Button 4: %d\n", decoder_66_bit_values[60]);
                printf("[59-32] Serial: ");
                for (signed int r = 59; r >= 32; r--)
                {
                    printf("%d", decoder_66_bit_values[r]);
                }
                printf("\n");
                printf("[31-0] Encrypted Code Data: ");
                for (signed int r = 31; r >= 0; r--)
                {
                    printf("%d", decoder_66_bit_values[r]);
                }
                printf("\n\n");
            }
}


void rf_rx_task()
{
    uint8_t decoded_serial_buffer[28];
    uint8_t validation_seq_num = 0;  // Quantos seriais foram validados até o momento
    uint8_t validation_seq_timeout_counter = 0; // Contador para invalidar a sequencia por timeout
    uint8_t validation_seq_timeout_max = 150; // Tempo de timeout em ms
    uint8_t validation_seq_timeout_period = 10; // Período


    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pin_bit_mask = (1ULL << GREEN_OK_LED_PIN),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE};
    gpio_config(&io_conf);

    while (1)
    {
        vTaskDelay((validation_seq_timeout_period) / portTICK_PERIOD_MS);

        if (xQueueReceive(log_evt_queue, &log_msg, 0))
        {
            ESP_LOGI(TAG, "%s", log_msg);
        }

        if (decoder_66bit_finish)
        {
            decoder_66bit_finish = 0;
            validation_seq_timeout_counter = 0; // Reseta o contador
            print_decoded_data();

            if (validation_seq_num == 0) // Se não está na rotina de validação da seq, inicie-a
            { 
                validation_seq_num++;
                validation_seq_timeout_counter++;
                // Guarda o serial no buffer
                for (int r = 59; r >= 32; r--)
                {
                    decoded_serial_buffer[r - 32] = decoder_66_bit_values[r];
                }
            }

            for (int r = 59; r >= 32; r--) // Verifica se o pacote atual tem o mesmo serial do primeiro e reseta validation se for diferente 
            {
                if (decoded_serial_buffer[r - 32] != decoder_66_bit_values[r])
                {
                    printf ("Serials in sequence don't match\n\n");
                    validation_seq_timeout_counter = 0;
                    validation_seq_num = 0;
                }
            }

            if (validation_seq_num >= NUM_OF_RX_TO_VALIDADE) // Se recebeu N pacotes ok em intervalos de 150ms
            { 
                set_last_rx_serial(decoded_serial_buffer);
                set_last_buttons_pressed();
                if (get_reg_mode()) // Se está no modo de registro de TX
                {
                    set_commands_valid_tx();
                }
                
                if (!get_reg_mode())  // Se não está no modo de registro de TX
                {
                    tx_match_execute(); // Verifica o match de TX e executa a ação
                }
                //printf ("111\n");
                validation_seq_timeout_counter = 0;
                validation_seq_num = 0;
                decoded_serial_buffer[27] = 2;  // "Apaga" (invalida) o bufer
                if (verbose_mode >= 1)
                {
                    printf("Received %d identical TX serials in intervals < 150ms\n", NUM_OF_RX_TO_VALIDADE);
                    printf("serial: %s  ", get_last_rx_serial());
                    printf("buttons: %d\n\n", get_last_buttons_pressed());
                }
                //printf ("222\n");
                gpio_set_level(GREEN_OK_LED_PIN, 1);
                vTaskDelay(OK_LED_TIME_MS / portTICK_PERIOD_MS);
                gpio_set_level(GREEN_OK_LED_PIN, 0);
                //printf ("333\n");
            }

            if (validation_seq_num > 0) // Se está na validação de uma seq de seriais
            validation_seq_num++;
            //printf ("444\n");
        }

        if (validation_seq_timeout_counter > 0)
            validation_seq_timeout_counter++;

        if (validation_seq_timeout_counter > validation_seq_timeout_max/validation_seq_timeout_period){
            validation_seq_timeout_counter = 0;
            validation_seq_num = 0;
            decoded_serial_buffer[27] = 2;  // "Apaga" (invalida) o bufer
            printf("Decode sequence Timeout: %dms\n\n", validation_seq_timeout_max);
        }        
    }
    //printf ("555\n");
    
}

void IRAM_ATTR rf_rx_gpio_handler(void *arg)
{
    switch (decoder_state)
    {
    case DECODER_S_PREAMBLE:
        gptimer_get_raw_count(rf_rx_timer, &current_time);
        if (last_edge_time > 0)
        {
            pulse_duration = current_time - last_edge_time;
            if (pulse_duration >= PREAMBLE_PULSE_MIN_US && pulse_duration <= PREAMBLE_PULSE_MAX_US)
            {
                preamble_pulse_count++;
                if (preamble_pulse_count >= PREAMBLE_EDGE_COUNT)
                {
                    if (verbose_mode >= 1)
                    {
                        xQueueSendFromISR(log_evt_queue, "Received preamble OK", NULL);
                    }
                    decoder_state = DECODER_S_THEADER;
                    preamble_pulse_count = 0;                    
                }
            }
            else // preamble pulse out of duty cycle range
            {
                preamble_pulse_count = 0;                
            }
        }
        last_edge_time = current_time;
        break;

    case DECODER_S_THEADER:
        if (gpio_get_level(RF_RX_PIN)) // if positive border
        {
            gptimer_get_raw_count(rf_rx_timer, &current_time);
            pulse_duration = current_time - last_edge_time;
            if (pulse_duration >= THEADER_MIN_US && pulse_duration <= THEADER_MAX_US)
            {                
                decoder_state = DECODER_S_66_BITS;                
                break;
            }
            else // Theader lenght wrong
            {                
                decoder_state = DECODER_S_PREAMBLE;
            }
        }
        else
        {
            decoder_state = DECODER_S_PREAMBLE;
        }

        last_edge_time = current_time;
        break;

    case DECODER_S_66_BITS:
        gptimer_get_raw_count(rf_rx_timer, &current_time);
        pulse_duration = current_time - last_edge_time;
        decoder_66_bit_status = gpio_get_level(RF_RX_PIN);

        switch (decoder_66_bit_status)
        {
        case 0:          
            if (last_edge_time > 0)
            {
                if (pulse_duration < 500)
                {
                    decoder_66_bit_values[decoder_66_bit_position] = 1;
                    decoder_66_bit_position++;
                }
                else
                {
                    decoder_66_bit_values[decoder_66_bit_position] = 0;
                    decoder_66_bit_position++;                   
                }
            }
            break;

        case 1:
            if (last_edge_time > 0)
            {
                if ((pulse_duration > 200) && (pulse_duration < 1000))
                {
                }
                else
                {
                    decoder_state = DECODER_S_PREAMBLE;
                }
            }

            break;               
        }

        last_edge_time = current_time;

        if (decoder_66_bit_position >= DECODER_BITS)
        {
            decoder_state = DECODER_S_PREAMBLE;
            decoder_66_bit_position = 0;
            decoder_66bit_finish = 1;
            if (verbose_mode >= 1)
            {
                xQueueSendFromISR(log_evt_queue, "DECODER finish", NULL);
            }
        }
    }
}

void initialize_rf_rx_pin(){
 gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << RF_RX_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(RF_RX_PIN, rf_rx_gpio_handler, (uint64_t*)RF_RX_PIN);
}


static void initialize_nvs_variables_to_ram_rfrx(){
    verbose_mode = get_nvs_verb_mode();
    rx_rele1_cycles = get_nvs_rx_rele1_cycles();
    rx_rele2_cycles = get_nvs_rx_rele2_cycles();
}


void initialize_rfrx(){
    initialize_rf_rx_timer(); 
    initialize_rf_rx_pin();
    create_rfrx_queues();
    initialize_nvs_variables_to_ram_rfrx();
    lcd_refresh_counter();
    lcd_refresh_rele_onoff();
}


void rele_task(){
    // Configura os pinos dos reles
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL << RELE1_PIN) | (1ULL << RELE2_PIN));
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    while (1)
    {

        if (rele_on)
        {
            gpio_set_level(RELE1_PIN, 1);
            vTaskDelay(RELE_DURATION_MS / portTICK_PERIOD_MS);
            gpio_set_level(RELE1_PIN, 0);

            vTaskDelay(RELE_INTERVAL_MS / portTICK_PERIOD_MS);
        }
        if (rele_on)
        {
            gpio_set_level(RELE2_PIN, 1);
            vTaskDelay(RELE_DURATION_MS / portTICK_PERIOD_MS);
            gpio_set_level(RELE2_PIN, 0);

            vTaskDelay(RELE_INTERVAL_MS / portTICK_PERIOD_MS);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
