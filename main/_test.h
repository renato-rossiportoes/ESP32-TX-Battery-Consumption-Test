#pragma once

void        reset_button_cycles();
void        increment_button_cycles(uint8_t button);
void        set_button_cycles (uint8_t button, uint32_t cycles);
uint32_t    get_button_cycles (uint8_t button);
void        set_test_onoff(uint8_t value);
uint8_t     get_test_onoff();
void        test_task ();