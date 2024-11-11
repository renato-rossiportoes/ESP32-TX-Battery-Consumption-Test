#include "nvs.h"
#include "nvs_flash.h"
#include "cmd_nvs.h"
#include "esp_vfs_fat.h"
#include "driver/uart_vfs.h"
#include "esp_partition.h"

#include "esp_console.h"
#include "_storage.h"
#include "_rfrx.h"
#include "__config.h"

static const char* TAG = "_storage";

nvs_handle_t tx_nvs_handle;


int8_t get_nvs_verb_mode(){
    nvs_handle_t handle;
    int8_t mode = 0;
    nvs_open("tx", NVS_READWRITE, &handle);
    nvs_get_i8(handle, "verbose_mode" , &mode);
    nvs_close(handle);
    return mode;
}

void set_nvs_verb_mode(uint8_t value){
    nvs_handle_t handle;
    nvs_open("tx", NVS_READWRITE, &handle);
    nvs_set_i8(handle, "verbose_mode" , value);
    nvs_close(handle);
}


void set_nvs_rx_rele1_cycles(uint64_t value)
{   
    int err = 0;    
    nvs_open("tx", NVS_READWRITE, &tx_nvs_handle);
    err = nvs_set_u64(tx_nvs_handle, "rx_rele1_cycles", value);
    printf("nvs_set_u64 rx_rele1_cycles: %s \n", esp_err_to_name(err));
    nvs_close(tx_nvs_handle);
}

void set_nvs_rx_rele2_cycles(uint64_t value)
{   
    int err = 0;    
    nvs_open("tx", NVS_READWRITE, &tx_nvs_handle);
    err = nvs_set_u64(tx_nvs_handle, "rx_rele2_cycles", value);
    printf("nvs_set_u64 rx_rele2_cycles: %s \n", esp_err_to_name(err));
    nvs_close(tx_nvs_handle);
}

uint64_t get_nvs_rx_rele1_cycles(){
    int err = 0;
    uint64_t value = 0;
    nvs_open("tx", NVS_READWRITE, &tx_nvs_handle);
    err = nvs_get_u64(tx_nvs_handle, "rx_rele1_cycles", &value);
    printf("nvs_get_u64 rx_rele1_cycles: %s \n", esp_err_to_name(err));
    nvs_close(tx_nvs_handle);
    return value;
}

uint64_t get_nvs_rx_rele2_cycles(){
    int err = 0;
    uint64_t value = 0;
    nvs_open("tx", NVS_READWRITE, &tx_nvs_handle);
    err = nvs_get_u64(tx_nvs_handle, "rx_rele2_cycles", &value);
    printf("nvs_get_u64 rx_rele2_cycles: %s \n", esp_err_to_name(err));
    nvs_close(tx_nvs_handle);
    return value;
}


void register_tx(int slot, int rele_num, int button, char serial[28])
{    
    if (slot > TX_SLOTS_NUM)
    {
        printf("Error: highest slot number is %d \n", TX_SLOTS_NUM);
        return;
    }

    printf("STORED TX: slot=%d rele_num=%d button=%d serial=%s\n", slot, rele_num, button, serial);
    static char TXxx[6];
    snprintf (TXxx, sizeof(TXxx), "tx%02d", (uint8_t)slot);
    TXxx[5]=0; //Garante que só tem 4 caracteres
    static char tx_string[35];
    sprintf (tx_string, "%02d%02d %01d%s", slot, rele_num, button, serial);
    nvs_open("tx", NVS_READWRITE, &tx_nvs_handle);
    printf("String to be written to %s key: %s \n", TXxx, tx_string);
    int err = 0;
    esp_console_run("nvs_namespace tx", &err); // Set namespace to "tx"
    printf ("nvs_namespace tx: %s \n", esp_err_to_name(err));
    err = nvs_set_str(tx_nvs_handle, TXxx, tx_string);
    nvs_close(tx_nvs_handle);
    printf ("nvs_set_str: %s \n", esp_err_to_name(err));
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
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &tx_nvs_handle);
    uint8_t error = 0;

    if (err != ESP_OK)
    {
        printf("Error opening namespace '%s': %s\n", namespace, esp_err_to_name(err));
        error = 2;
    }

    int8_t value;
    err = nvs_get_i8(tx_nvs_handle, key, &value);

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

    nvs_close(tx_nvs_handle);
    return error;
}



static uint8_t check_u64_key_exists(const char *namespace, const char *key)
{
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &tx_nvs_handle);
    uint8_t error = 0;

    if (err != ESP_OK)
    {
        printf("Error opening namespace '%s': %s\n", namespace, esp_err_to_name(err));
        error = 2;
    }

    uint64_t value;
    err = nvs_get_u64(tx_nvs_handle, key, &value);

    if (err == ESP_OK)
    {
        printf("Key '%s' exists in the namespace '%s' and its value is: %llu\n", key, namespace, value);
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

    nvs_close(tx_nvs_handle);
    return error;
}



static uint8_t check_key_exists(const char *namespace, const char *key) {
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &tx_nvs_handle);
    uint8_t error = 0;

    if (err != ESP_OK) {
        printf("Error opening namespace '%s': %s\n", namespace, esp_err_to_name(err));
        error = 2;
    }

    char value[35]; 
    size_t len = 35;
    err = nvs_get_str(tx_nvs_handle, key, value, &len);

    if (err == ESP_OK) {
        printf("Key '%s' exists in the namespace '%s' and its value is: %s\n", key, namespace, value);
        error = 1;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("Key '%s' does not exist in the namespace '%s'.\n", key, namespace);
        error = 0;
    } else {
        printf("Error accessing key '%s': %s\n", key, esp_err_to_name(err));
        error = 2;
    }

    nvs_close(tx_nvs_handle);
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


void initialize_tx_namespace(){
    nvs_open("tx", NVS_READWRITE, &tx_nvs_handle);

    int err = 0;
    nvs_partition_search();
    esp_console_run("nvs_namespace tx", &err); // Set namespace to "tx"
    printf ("command ""nvs_namespace tx"": %s \n", esp_err_to_name(err));
    
    
    static char TXxx[5];
    for (uint8_t j=1; j <= TX_SLOTS_NUM; j++){
        sprintf (TXxx, "tx%02d", j);
        if (check_key_exists("tx", TXxx) == 0){
            printf ("Creating entry %s - ", TXxx);
            nvs_open("tx", NVS_READWRITE, &tx_nvs_handle);
            err = nvs_set_str (tx_nvs_handle, TXxx, "1234567890123456789012345678901234");
            nvs_close(tx_nvs_handle);
            printf ("%s \n", esp_err_to_name(err));
        }
    }
    
    // Verifica se existe as chaves rx_rele1_cycles e rx_rele2 _cycles. Se não tiver, cria com valor = 0
    if (check_u64_key_exists("tx", "rx_rele1_cycles") == 0)
    {
        printf("Creating entry %s - ", "rx_rele1_cycles");
        nvs_open("tx", NVS_READWRITE, &tx_nvs_handle);
        err = nvs_set_u64(tx_nvs_handle, "rx_rele1_cycles", 0);
        nvs_close(tx_nvs_handle);
        printf("%s \n", esp_err_to_name(err));
    }
    if (check_u64_key_exists("tx", "rx_rele2_cycles") == 0)
    {
        printf("Creating entry %s - ", "rx_rele2_cycles");
        nvs_open("tx", NVS_READWRITE, &tx_nvs_handle);
        err = nvs_set_u64(tx_nvs_handle, "rx_rele2_cycles", 0);
        nvs_close(tx_nvs_handle);
        printf("%s \n", esp_err_to_name(err));
    }


    // Verifica se existe a chave verbose_mode. Se não tiver, cria com valor = 0
    if (check_i8_key_exists("tx", "verbose_mode") == 0)
    {
        printf("Creating entry %s - ", "verbose_mode");
        nvs_open("tx", NVS_READWRITE, &tx_nvs_handle);
        err = nvs_set_i8(tx_nvs_handle, "verbose_mode", 0);
        nvs_close(tx_nvs_handle);
        printf("%s \n", esp_err_to_name(err));
    }

    nvs_close(tx_nvs_handle);
}



