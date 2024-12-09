#pragma once

#include "nvs.h"
#include "nvs_flash.h"

//nvs_handle_t my_nvs_handle;

void        initialize_nvs(void);
void        initialize_filesystem(void);
void        initialize_namespace();

void        button_cycles_nvs_to_ram();
int8_t      get_nvs_verb_mode();
void        set_nvs_verb_mode(uint8_t value);
void        reset_nvs_button_cycles();
void        set_nvs_button_cycles(uint8_t button, uint32_t value);
uint32_t    get_nvs_button_cycles(uint8_t button);
