
#include "rgb.h"

#include "driver/gpio.h"
#include "esp_log.h"

// RGB pinovi
#define RGB_RED   GPIO_NUM_17
#define RGB_GREEN GPIO_NUM_18
#define RGB_BLUE  GPIO_NUM_21

// 0 = zajednicka katoda
// 1 = zajednicka anoda
#define RGB_COMMON_ANODE 0

static const char *TAG = "RGB";

// Postavljanje stanja jedne boje
static void rgb_write(gpio_num_t pin, bool on)
{
    int level = on ? 1 : 0;

    if (RGB_COMMON_ANODE)
    {
        level = !level;
    }

    gpio_set_level(pin, level);
}

// Ukljucivanje odabranih boja
void rgb_set(bool red, bool green, bool blue)
{
    rgb_write(RGB_RED, red);
    rgb_write(RGB_GREEN, green);
    rgb_write(RGB_BLUE, blue);
}

// Gasenje RGB diode
void rgb_off(void)
{
    rgb_set(false, false, false);
}

// Inicijalizacija RGB diode
void rgb_init(void)
{
    gpio_config_t rgb_config = {
        .pin_bit_mask = (1ULL << RGB_RED) |
                        (1ULL << RGB_GREEN) |
                        (1ULL << RGB_BLUE),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&rgb_config));

    rgb_off();

    ESP_LOGI(TAG, "RGB dioda inicijalizovana");
}
