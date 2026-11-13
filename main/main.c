
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "pir.h"
#include "buzzer.h"
#include "rfid.h"
#include "lcd.h"
#include "rgb.h"
#include "card_storage.h"

#define ACCESS_TIME 15000
#define SCAN_TIME   20000

static const char *TAG = "MAIN";

typedef enum { LOCKED, UNLOCKED, ALARM } state_t;
typedef enum { NORMAL, REGISTER, DELETE } mode_t;

static state_t state = LOCKED;
static mode_t mode = NORMAL;

static int64_t access_end = 0;
static int64_t scan_end = 0;

// Prikaz teksta na LCD-u
static void lcd_message(const char *line1, const char *line2)
{
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print(line1);
    lcd_set_cursor(1, 0);
    lcd_print(line2);
}

// Promjena stanja sistema
static void set_state(state_t new_state)
{
    state = new_state;
    ESP_ERROR_CHECK(buzzer_off());

    if (state == LOCKED)
    {
        rgb_set(false, false, true);
        lcd_message("SISTEM AKTIVAN", "Prisloni karticu");
    }
    else if (state == UNLOCKED)
    {
        rgb_set(false, true, false);
        lcd_message("PRISTUP ODOBREN", "Alarm iskljucen");
    }
    else
    {
        rgb_set(true, false, false);
        lcd_message("ALARM!", "Pokret otkriven");
        ESP_ERROR_CHECK(buzzer_on());
    }
}

// Obrada komandi iz terminala
static void command_run(const char *cmd, int64_t now)
{
    if (strcmp(cmd, "register") == 0)
    {
        mode = REGISTER;
        scan_end = now + SCAN_TIME;
        ESP_ERROR_CHECK(buzzer_off());
        lcd_message("REGISTRACIJA", "Prisloni karticu");
    }
    else if (strcmp(cmd, "delete") == 0)
    {
        mode = DELETE;
        scan_end = now + SCAN_TIME;
        ESP_ERROR_CHECK(buzzer_off());
        lcd_message("BRISANJE", "Prisloni karticu");
    }
    else if (strcmp(cmd, "list") == 0)
    {
        cards_list();
    }
    else if (strcmp(cmd, "start") == 0)
    {
        mode = NORMAL;
        set_state(LOCKED);
    }
    else if (strcmp(cmd, "help") == 0)
    {
        ESP_LOGI(TAG, "Komande: register, delete, list, start, help");
    }
    else
    {
        ESP_LOGW(TAG, "Nepoznata komanda: %s", cmd);
    }
}

// Citanje komandi preko UART-a
static void terminal_read(int64_t now)
{
    static char cmd[32];
    static int len = 0;
    uint8_t ch;

    while (uart_read_bytes(UART_NUM_0, &ch, 1, 0) == 1)
    {
        if (ch == '\r' || ch == '\n')
        {
            if (len > 0)
            {
                cmd[len] = '\0';
                command_run(cmd, now);
                len = 0;
            }
        }
        else if (ch == 8 || ch == 127)
        {
            if (len > 0) len--;
        }
        else if (ch >= 32 && ch <= 126 && len < 31)
        {
            cmd[len++] = (char)ch;
        }
    }
}

// Glavni program
void app_main(void)
{
    pir_init();
    ESP_ERROR_CHECK(buzzer_init());
    rgb_init();

    ESP_ERROR_CHECK(lcd_init());
    ESP_ERROR_CHECK(rc522_init());
    ESP_ERROR_CHECK(cards_init());

    ESP_ERROR_CHECK(uart_driver_install(
        UART_NUM_0, 1024, 0, 0, NULL, 0
    ));

    set_state(LOCKED);
    ESP_LOGI(TAG, "Sistem pokrenut");
    ESP_LOGI(TAG, "Komande: register, delete, list, start, help");

    uint8_t uid[4];
    uint8_t last_uid[4] = {0};
    int64_t last_read = -2000;
    bool last_valid = false;
    bool previous_motion = pir_motion_detected();

    while (1)
    {
        int64_t now = esp_timer_get_time() / 1000;
        terminal_read(now);

        // Istek registracije
        if (mode != NORMAL && now >= scan_end)
        {
            mode = NORMAL;
            set_state(LOCKED);
            ESP_LOGW(TAG, "Vrijeme isteklo");
        }

        // RFID kartica
        if (rc522_read_uid(uid))
        {
            bool repeated = last_valid &&
                memcmp(uid, last_uid, 4) == 0 &&
                now - last_read < 1500;

            if (!repeated || mode != NORMAL)
            {
                memcpy(last_uid, uid, 4);
                last_valid = true;
                last_read = now;

                if (mode == REGISTER || mode == DELETE)
                {
                    esp_err_t err = (mode == REGISTER) ?
                        cards_add(uid) : cards_remove(uid);

                    ESP_LOGI(TAG, "Kartica: %s",
                             esp_err_to_name(err));

                    if (err == ESP_OK)
                    {
                        lcd_message("KARTICA", "Sacuvana/obrisana");
                    }

                    mode = NORMAL;
                    vTaskDelay(pdMS_TO_TICKS(800));
                    set_state(LOCKED);
                }
                else if (cards_contains(uid))
                {
                    access_end = now + ACCESS_TIME;
                    set_state(UNLOCKED);
                }
                else
                {
                    ESP_LOGW(TAG, "Pristup odbijen");

                    if (state == LOCKED)
                    {
                        lcd_message("PRISTUP ODBIJEN", "Nepoznata kartica");
                    }
                }
            }
        }

        // Istek pristupa
        if (mode == NORMAL &&
            state == UNLOCKED && now >= access_end)
        {
            set_state(LOCKED);
        }

        // PIR alarm
        bool motion = pir_motion_detected();

        if (mode == NORMAL && state == LOCKED &&
            motion && !previous_motion)
        {
            set_state(ALARM);
            ESP_LOGW(TAG, "Pokret detektovan!");
        }

        previous_motion = motion;

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
