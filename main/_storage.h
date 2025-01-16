#pragma once

#include "nvs.h"
#include "nvs_flash.h"

//nvs_handle_t tx_nvs_handle;

void        initialize_nvs(void);
void        initialize_filesystem(void);
void        initialize_tx_namespace();
void        register_tx (int slot, int rele_num, int button, char serial[28]);


int8_t      get_nvs_verb_mode();
void        set_nvs_verb_mode(uint8_t value);
void        set_nvs_rx_rele1_cycles(uint64_t value);
void        set_nvs_rx_rele2_cycles(uint64_t value);
uint64_t    get_nvs_rx_rele1_cycles();
uint64_t    get_nvs_rx_rele2_cycles();
