
#ifndef LCD_H
#define LCD_H

#include "esp_err.h"

esp_err_t lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(int row, int col);
void lcd_print(const char *text);

#endif
