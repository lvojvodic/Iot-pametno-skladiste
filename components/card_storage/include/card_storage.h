
#ifndef CARD_STORAGE_H
#define CARD_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define MAX_CARDS 20

esp_err_t cards_init(void);
bool cards_contains(const uint8_t uid[4]);
esp_err_t cards_add(const uint8_t uid[4]);
esp_err_t cards_remove(const uint8_t uid[4]);
void cards_list(void);

#endif
