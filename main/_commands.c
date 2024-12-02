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
#include "_test.h"
#include "__config.h"



// Struct para o comando de ligar o teste
static struct {
    struct arg_str *onoff;        // on off
    struct arg_end *end;
} test_args;


// Struct para o comando de configurar verbosidade do terminal
static struct {
    struct arg_int *mode;   // Modo de verbosidade dos RXs recebidos
    struct arg_end *end;
} verb_args;

// Struct para o comando de deletar os dados salvos
static struct {
    struct arg_end *end;
} nvs_reset_args;



static int test_func(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&test_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, test_args.end, argv[0]);
        return 1;
    }
    
    const char *onoff33 = test_args.onoff->sval[0];

    printf ("Foi digitado: %s\n", onoff33);

    if (strcmp(onoff33, "on") == 0)
    {
        set_test_onoff(1);
        printf("Test ON\n");
        return ESP_OK;
    }

    if (strcmp(onoff33, "off") == 0)
    {
        set_test_onoff(0);
        printf("Test OFF\n");
        return ESP_OK;
    }
    printf("invalid argument\n");
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
        //nvs_close(my_nvs_handle);
        printf("invalid mode\n");
        return ESP_FAIL;
    }
    //nvs_set_i8(my_nvs_handle, "verbose_mode", mode);
    set_nvs_verb_mode (mode);
    printf ("Verbose Mode: %d\n", mode);
    //nvs_close(my_nvs_handle);
    return ESP_OK;
}


static int nvs_reset_func(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&nvs_reset_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, nvs_reset_args.end, argv[0]);
        return 1;
    }
    printf("Erasing all NVS Test data\n");
    int err = 0;
    esp_console_run("nvs_erase_namespace test", &err);
    printf("%s \n", esp_err_to_name(err));
    return err;
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



void register_nvs_reset(void)
{
    nvs_reset_args.end = arg_end(2);
    const esp_console_cmd_t nvs_reset_cmd = {
        .command = "nvs_reset",
        .help = "Erase all TX data\n",
        .hint = NULL,
        .func = &nvs_reset_func,
        .argtable = &nvs_reset_args};

    ESP_ERROR_CHECK(esp_console_cmd_register(&nvs_reset_cmd));
}



void register_test(void)
{
    test_args.onoff = arg_str1(NULL, NULL, "<on|off>", "on | off");
    test_args.end = arg_end(2);
    
    const esp_console_cmd_t test_cmd = {
        .command = "test",
        .help = "Turn test on or off\n",
        .hint = NULL,
        .func = &test_func,
        .argtable = &test_args
    };    

    ESP_ERROR_CHECK(esp_console_cmd_register(&test_cmd));
}