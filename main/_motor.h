#pragma once

//extern gptimer_handle_t timer;

void initialize_motor();
void motor_start();
void motor_stop();
void motor_button_test_cycle(uint8_t direction);
void motor_timer_start();
void motor_timer_stop();