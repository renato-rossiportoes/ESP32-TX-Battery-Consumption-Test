#pragma once

void register_verb(void);
void register_reg(void);
void register_tx_reset(void);
void register_rele(void);
void set_commands_valid_tx();
void set_reg_mode(uint8_t mode);
uint8_t get_reg_mode();