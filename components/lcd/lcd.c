
#include "lcd.h"

#include <stdint.h>
#include <stddef.h>

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

// LCD pinovi
#define LCD_SDA GPIO_NUM_8
#define LCD_SCL GPIO_NUM_9
#define LCD_ADDRESS 0x27

// Kontrolni bitovi LCD-a
#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_BL 0x08

static const char *TAG = "LCD";

static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t lcd_device;

// Slanje podataka preko I2C
static void lcd_write(const uint8_t *data, size_t length)
{
    ESP_ERROR_CHECK(i2c_master_transmit(
        lcd_device, data, length, 1000
    ));
}

// Slanje 4 bita LCD-u
static void lcd_send_nibble(uint8_t data, uint8_t mode)
{
    uint8_t value = (data & 0xF0) | LCD_BL | mode;

    uint8_t buffer[3] = {
        value,
        (uint8_t)(value | LCD_EN),
        value
    };

    lcd_write(buffer, sizeof(buffer));
    esp_rom_delay_us(100);
}

// Slanje jednog bajta
static void lcd_send_byte(uint8_t value, uint8_t mode)
{
    lcd_send_nibble(value & 0xF0, mode);
    lcd_send_nibble((value << 4) & 0xF0, mode);
}

// Slanje LCD komande
static void lcd_command(uint8_t command)
{
    lcd_send_byte(command, 0);

    if (command == 0x01 || command == 0x02)
    {
        vTaskDelay(pdMS_TO_TICKS(3));
    }
}

// Brisanje ekrana
void lcd_clear(void)
{
    lcd_command(0x01);
}

// Postavljanje kursora
void lcd_set_cursor(int row, int col)
{
    if (row < 0 || row > 1 || col < 0 || col > 15)
    {
        return;
    }

    uint8_t address = (row == 0) ? 0x00 : 0x40;

    lcd_command(0x80 | (address + col));
}

// Ispis teksta
void lcd_print(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    while (*text != '\0')
    {
        lcd_send_byte((uint8_t)*text, LCD_RS);
        text++;
    }
}

// Inicijalizacija LCD displeja
esp_err_t lcd_init(void)
{
    // Konfiguracija I2C magistrale
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = LCD_SDA,
        .scl_io_num = LCD_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };

    esp_err_t err = i2c_new_master_bus(
        &bus_config, &i2c_bus
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C inicijalizacija nije uspjela");
        return err;
    }

    // Provjera LCD adrese
    err = i2c_master_probe(i2c_bus, LCD_ADDRESS, 1000);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "LCD nije pronadjen");
        return err;
    }

    // Dodavanje LCD uredjaja
    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_ADDRESS,
        .scl_speed_hz = 100000
    };

    err = i2c_master_bus_add_device(
        i2c_bus, &device_config, &lcd_device
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Dodavanje LCD-a nije uspjelo");
        return err;
    }

    // Inicijalizacija HD44780 kontrolera
    vTaskDelay(pdMS_TO_TICKS(100));

    lcd_send_nibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    lcd_send_nibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    lcd_send_nibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Prelazak u 4-bitni rezim
    lcd_send_nibble(0x20, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Podesavanje LCD-a
    lcd_command(0x28);
    lcd_command(0x08);
    lcd_clear();
    lcd_command(0x06);
    lcd_command(0x0C);

    ESP_LOGI(TAG, "LCD inicijalizovan");

    return ESP_OK;
}
