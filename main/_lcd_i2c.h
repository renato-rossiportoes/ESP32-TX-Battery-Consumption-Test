#pragma once

void initialize_i2c();
void lcd_init();
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_write_string(const char *str);
void i2c_scanner();