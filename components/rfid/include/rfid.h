
#ifndef RFID_H
#define RFID_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

esp_err_t rc522_init(void);
bool rc522_read_uid(uint8_t uid[4]);

#endif
