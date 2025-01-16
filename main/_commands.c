#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "cmd_nvs.h"
#include "nvs.h"

#include "_commands.h"
#include "_storage.h"
#include "_rfrx.h"
#include "__config.h"


static uint8_t commands_valid_tx = 0;
static uint8_t reg_mode = 0;


// Struct para o comando de ligar os ciclos de rele
static struct {
    struct arg_str *onoff;        // on off
    struct arg_end *end;
} rele_args;


// Struct para o comando de registrar TX
static struct {
    struct arg_int *slot;       // tx position
    struct arg_int *rele_num;   // número do rele associado.
    struct arg_end *end;
} reg_args;

// Struct para o comando de configurar verbosidade do terminal
static struct {
    struct arg_int *mode;   // Modo de verbosidade dos RXs recebidos
    struct arg_end *end;
} verb_args;

// Struct para o comando de deletar todos os TX
static struct {
    struct arg_end *end;
} tx_reset_args;



void set_commands_valid_tx(){
    commands_valid_tx = 1;
}

uint8_t get_commands_valid_tx(){
    return commands_valid_tx;
}


void reset_commands_valid_tx(){
    commands_valid_tx = 0;
}

void set_reg_mode(uint8_t mode){
    if (mode) reg_mode = 1;
    if (!mode) reg_mode = 0;
}

uint8_t get_reg_mode(){
    return reg_mode;
}


static int rele_func(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&rele_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, rele_args.end, argv[0]);
        return 1;
    }
    
    const char *onoff33 = rele_args.onoff->sval[0];
    //int err = 0;
    //esp_console_run("nvs_namespace tx", &err); // Set namespace to "tx"

    printf ("Foi digitado: %s\n", onoff33);

    if (strcmp(onoff33, "on") == 0)
    {
        set_rele_onoff(1);
        printf("Rele ON\n");
        return ESP_OK;
    }

    if (strcmp(onoff33, "off") == 0)
    {
        set_rele_onoff(0);
        printf("Rele OFF\n");
        return ESP_OK;
    }
    printf("invalid value\n");
    return ESP_FAIL;
}


static int verb_func(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&verb_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, verb_args.end, argv[0]);
        return 1;
    }

    const int mode = verb_args.mode->ival[0];
    int err = 0;
    esp_console_run("nvs_namespace tx", &err); // Set namespace to "tx"

    if (mode < 0 || mode > 2)
    {
        //nvs_close(tx_nvs_handle);
        printf("invalid mode\n");
        return ESP_FAIL;
    }
    //nvs_set_i8(tx_nvs_handle, "verbose_mode", mode);
    set_nvs_verb_mode (mode);
    printf ("Verbose Mode: %d\n", mode);
    //nvs_close(tx_nvs_handle);
    return ESP_OK;
}


static int tx_reset_func(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&tx_reset_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, tx_reset_args.end, argv[0]);
        return 1;
    }
    printf("Erasing all TX data\n");
    int err = 0;
    esp_console_run("nvs_erase_namespace tx", &err); // Set namespace to "tx"
    printf("%s \n", esp_err_to_name(err));
    return err;
}


static int reg_func(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &reg_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, reg_args.end, argv[0]);
        return 1;
    }

    const int slot = reg_args.slot->ival[0];
    const int rele_num = reg_args.rele_num->ival[0];
    set_reg_mode(1);
    printf ("REGISTER - Waiting %dms for a TX...\n", TX_REG_TIMEOUT_MS);
    for (uint16_t r=0; r<TX_REG_TIMEOUT_MS/10; r++){
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if (get_commands_valid_tx()){
            reset_commands_valid_tx();
            register_tx(slot, rele_num, get_last_buttons_pressed(), get_last_rx_serial()); // Registra último serial validado no slot indicado
            set_reg_mode(0);
            return ESP_OK;
        }
    }
    printf ("Timeout\n");
    reset_commands_valid_tx();
    set_reg_mode(0);
    return ESP_ERR_TIMEOUT;
}



void register_verb(void)
{
    verb_args.mode = arg_int1(NULL, NULL, "<verbose mode>", "[0-1]");
    verb_args.end = arg_end(2);
    
    const esp_console_cmd_t verb_cmd = {
        .command = "verb",
        .help = "Verbose Mode\n",
        .hint = NULL,
        .func = &verb_func,
        .argtable = &verb_args
    };    

    ESP_ERROR_CHECK(esp_console_cmd_register(&verb_cmd));
}


void register_tx_reset(void)
{
    tx_reset_args.end = arg_end(2);
    const esp_console_cmd_t tx_reset_cmd = {
        .command = "tx_reset",
        .help = "Erase all TX data\n",
        .hint = NULL,
        .func = &tx_reset_func,
        .argtable = &tx_reset_args};

    ESP_ERROR_CHECK(esp_console_cmd_register(&tx_reset_cmd));
}

void register_reg(void)
{
    reg_args.slot = arg_int1(NULL, NULL, "<slot>", "TX Slot [01-10]");
    reg_args.rele_num = arg_int1(NULL, NULL, "<relay>","Relay Number");
    reg_args.end = arg_end(2);
    
    const esp_console_cmd_t reg_cmd = {
        .command = "reg",
        .help = "Register TX by serial\n",
        .hint = NULL,
        .func = &reg_func,
        .argtable = &reg_args
    };    

    ESP_ERROR_CHECK(esp_console_cmd_register(&reg_cmd));
}



void register_rele(void)
{
    rele_args.onoff = arg_str1(NULL, NULL, "<on|off>", "on | off");
    rele_args.end = arg_end(2);
    
    const esp_console_cmd_t rele_cmd = {
        .command = "rele",
        .help = "Turn relay cycles on or off\n",
        .hint = NULL,
        .func = &rele_func,
        .argtable = &rele_args
    };    

    ESP_ERROR_CHECK(esp_console_cmd_register(&rele_cmd));
}