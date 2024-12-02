#pragma once

void        reset_button_cycles();
void        set_button_cycles (uint8_t button, uint64_t cycles);
uint64_t    get_button_cycles (uint8_t button);
void        set_test_onoff(uint8_t value);
uint8_t     get_test_onoff();
void        test_task ();