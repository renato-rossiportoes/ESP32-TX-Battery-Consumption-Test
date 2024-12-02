#pragma once

#include "nvs.h"
#include "nvs_flash.h"

//nvs_handle_t my_nvs_handle;

void        initialize_nvs(void);
void        initialize_filesystem(void);
void        initialize_namespace();
void        register_tx (int slot, int rele_num, int button, char serial[28]);


int8_t      get_nvs_verb_mode();
void        set_nvs_verb_mode(uint8_t value);
void        reset_nvs_button_cycles();
void        set_nvs_button_cycles(uint8_t button, uint64_t value);
uint64_t    get_nvs_button_cycles(uint8_t button);
