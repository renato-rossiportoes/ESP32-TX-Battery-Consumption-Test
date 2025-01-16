#pragma once

void        initialize_rfrx();
void        rf_rx_task();
char*       get_last_rx_serial();
uint8_t     get_last_buttons_pressed ();
void        set_rx_rele1_cycles(uint64_t value);
void        set_rx_rele2_cycles(uint64_t value);
uint64_t    get_rx_rele1_cycles();
uint64_t    get_rx_rele2_cycles();
void        rele_task();
void        set_rele_onoff(uint8_t value);
uint8_t     get_rele_onoff();