#include "nvs.h"
#include "nvs_flash.h"
#include "cmd_nvs.h"
#include "esp_vfs_fat.h"
#include "driver/uart_vfs.h"
#include "esp_partition.h"

#include "esp_console.h"
#include "_storage.h"
#include "_test.h"
#include "__config.h"

static const char* TAG = "_storage";

nvs_handle_t my_nvs_handle;


void button_cycles_nvs_to_ram()
{
    for (uint8_t r = 1; r <= NUMBER_OF_TEST_BUTTONS; r++)
    {
        set_button_cycles(r, get_nvs_button_cycles(r));
    }
}

int8_t get_nvs_verb_mode(){
    nvs_handle_t handle;
    int8_t mode = 0;
    nvs_open("test", NVS_READWRITE, &handle);
    nvs_get_i8(handle, "verbose_mode" , &mode);
    nvs_close(handle);
    return mode;
}

void set_nvs_verb_mode(uint8_t value){
    nvs_handle_t handle;
    nvs_open("test", NVS_READWRITE, &handle);
    nvs_set_i8(handle, "verbose_mode" , value);
    nvs_close(handle);
}


void reset_nvs_button_cycles(){
    int err = 0;
    nvs_open("test", NVS_READWRITE, &my_nvs_handle);
    char str_buffer[17];
    for (uint8_t i=1; i<=NUMBER_OF_TEST_BUTTONS; i++){
        snprintf (str_buffer, 16, "button%d_cycles", i);
        err = nvs_set_u32(my_nvs_handle, str_buffer, 0);
        printf("reset button%d_cycles: %s\n", i, esp_err_to_name(err));
    }
    nvs_close(my_nvs_handle);
}

void set_nvs_button_cycles(uint8_t button, uint32_t value){
    int err = 0;
    nvs_open("test", NVS_READWRITE, &my_nvs_handle);
    char str_buffer[17];
    sprintf (str_buffer, "button%d_cycles", button);
    err = nvs_set_u32(my_nvs_handle, str_buffer, value);
    printf("set button%d_cycles: %s\n", button, esp_err_to_name(err));
    nvs_close(my_nvs_handle);
}

uint32_t get_nvs_button_cycles(uint8_t button){
    int err = 0;
    uint32_t value = 0;
    nvs_open("test", NVS_READWRITE, &my_nvs_handle);
    char str_buffer[17];
    sprintf (str_buffer, "button%d_cycles", button);
    err = nvs_get_u32(my_nvs_handle, str_buffer, &value);
    printf("get button%d_cycles: %s\n", button, esp_err_to_name(err));
    nvs_close(my_nvs_handle);
    return value;
}

void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}


void initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }

    
}



static uint8_t check_i8_key_exists(const char *namespace, const char *key)
{
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &my_nvs_handle);
    uint8_t error = 0;

    if (err != ESP_OK)
    {
        printf("Error opening namespace '%s': %s\n", namespace, esp_err_to_name(err));
        error = 2;
    }

    int8_t value;
    err = nvs_get_i8(my_nvs_handle, key, &value);

    if (err == ESP_OK)
    {
        printf("Key '%s' exists in the namespace '%s' and its value is: %d\n", key, namespace, value);
        error = 1;
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        printf("Key '%s' does not exist in the namespace '%s'.\n", key, namespace);
        error = 0;
    }
    else
    {
        printf("Error accessing key '%s': %s\n", key, esp_err_to_name(err));
        error = 2;
    }

    nvs_close(my_nvs_handle);
    return error;
}



static uint8_t check_u32_key_exists(const char *namespace, const char *key)
{
    esp_err_t err = 0;
    //err = nvs_open(namespace, NVS_READONLY, &my_nvs_handle);
    uint8_t error = 0;

    if (err != ESP_OK)
    {
        printf("Error opening namespace '%s': %s\n", namespace, esp_err_to_name(err));
        error = 2;
    }

    uint32_t value;
    err = nvs_get_u32(my_nvs_handle, key, &value);

    if (err == ESP_OK)
    {
        printf("Key '%s' exists in the namespace '%s' and its value is: %lu\n", key, namespace, value);
        error = 1;
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        printf("Key '%s' does not exist in the namespace '%s'.\n", key, namespace);
        error = 0;
    }
    else
    {
        printf("Error accessing key '%s': %s\n", key, esp_err_to_name(err));
        error = 2;
    }

    //nvs_close(my_nvs_handle);
    return error;
}


void nvs_partition_search()
{
    printf("Searching for NVS partitions...\n");

    // Itera sobre todas as partições
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);

    if (it == NULL)
    {
        printf("No NVS partitions found.\n");
    }
    else
    {
        while (it != NULL)
        {
            // Obtém as informações da partição
            const esp_partition_t *partition = esp_partition_get(it);
            printf("Partition found: Name: %s, Type: %d, Subtype: %d, Size: %lu bytes\n",
                   partition->label, partition->type, partition->subtype, partition->size);

            // Próxima partição do mesmo tipo
            it = esp_partition_next(it);
        }
        // Libera o iterador
        esp_partition_iterator_release(it);
    }
}


void initialize_namespace(){
    nvs_open("test", NVS_READWRITE, &my_nvs_handle);

    int err = 0;
    nvs_partition_search();
    esp_console_run("nvs_namespace test", &err); // Set namespace to "test"
    printf ("command ""nvs_namespace test"": %s \n", esp_err_to_name(err));
    
    // Verifica se existe as chaves button[x]_cycles. Se não tiver, cria com valor = 0
    char str_buffer[17];
    for (uint8_t i = 1; i <= NUMBER_OF_TEST_BUTTONS; i++)
    {        
        sprintf(str_buffer, "button%d_cycles", i);
        if (check_u32_key_exists("test", str_buffer) == 0)
        {
            printf("Creating entry %s - ", str_buffer);
            err = nvs_set_u32(my_nvs_handle, str_buffer, 0);
            printf("%s \n", esp_err_to_name(err));
        }
    }

    // Verifica se existe a chave verbose_mode. Se não tiver, cria com valor = 0
    if (check_i8_key_exists("test", "verbose_mode") == 0)
    {
        printf("Creating entry %s - ", "verbose_mode");
        nvs_open("test", NVS_READWRITE, &my_nvs_handle);
        err = nvs_set_i8(my_nvs_handle, "verbose_mode", 0);
        nvs_close(my_nvs_handle);
        printf("%s \n", esp_err_to_name(err));
    }

    nvs_close(my_nvs_handle);
}



