
#include "rfid.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_check.h"

#include "rc522.h"
#include "rc522_picc.h"
#include "driver/rc522_spi.h"

// Pinovi RFID citaca
#define RFID_SCK  GPIO_NUM_12
#define RFID_MISO GPIO_NUM_13
#define RFID_MOSI GPIO_NUM_11
#define RFID_SS   GPIO_NUM_10
#define RFID_RST  GPIO_NUM_4

static const char *TAG = "RFID";

static QueueHandle_t uid_queue;
static rc522_driver_handle_t spi_driver;
static rc522_handle_t rfid_scanner;

// Konfiguracija SPI magistrale
static spi_bus_config_t spi_bus = {
    .miso_io_num = RFID_MISO,
    .mosi_io_num = RFID_MOSI,
    .sclk_io_num = RFID_SCK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1
};

// Konfiguracija RFID uredjaja
static rc522_spi_config_t rfid_config = {
    .host_id = SPI2_HOST,
    .bus_config = &spi_bus,
    .dev_config = {
        .spics_io_num = RFID_SS,
        .clock_speed_hz = 400000
    },
    .rst_io_num = RFID_RST
};

// Obrada detektovane kartice
static void on_card_detected(
    void *arg,
    esp_event_base_t base,
    int32_t event_id,
    void *data)
{
    (void)arg;
    (void)base;
    (void)event_id;

    rc522_picc_state_changed_event_t *event = data;

    if (event == NULL || event->picc == NULL)
    {
        return;
    }

    if (event->picc->state != RC522_PICC_STATE_ACTIVE &&
        event->picc->state != RC522_PICC_STATE_ACTIVE_H)
    {
        return;
    }

    // Prihvatamo kartice sa UID-om od 4 bajta
    if (event->picc->uid.length != 4)
    {
        ESP_LOGW(TAG, "Kartica nema UID od 4 bajta");
        return;
    }

    uint8_t card_uid[4];

    memcpy(card_uid, event->picc->uid.value, 4);

    xQueueOverwrite(uid_queue, card_uid);
}

// Inicijalizacija RFID citaca
esp_err_t rc522_init(void)
{
    uid_queue = xQueueCreate(1, sizeof(uint8_t[4]));

    if (uid_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(
        rc522_spi_create(&rfid_config, &spi_driver),
        TAG,
        "SPI inicijalizacija nije uspjela"
    );

    ESP_RETURN_ON_ERROR(
        rc522_driver_install(spi_driver),
        TAG,
        "RFID drajver nije instaliran"
    );

    rc522_config_t scanner_config = {
        .driver = spi_driver
    };

    ESP_RETURN_ON_ERROR(
        rc522_create(&scanner_config, &rfid_scanner),
        TAG,
        "Kreiranje RFID citaca nije uspjelo"
    );

    ESP_RETURN_ON_ERROR(
        rc522_register_events(
            rfid_scanner,
            RC522_EVENT_PICC_STATE_CHANGED,
            on_card_detected,
            NULL
        ),
        TAG,
        "RFID dogadjaj nije registrovan"
    );

    ESP_RETURN_ON_ERROR(
        rc522_start(rfid_scanner),
        TAG,
        "RFID citac nije pokrenut"
    );

    ESP_LOGI(TAG, "RFID citac spreman");

    return ESP_OK;
}

// Ocitavanje UID-a kartice
bool rc522_read_uid(uint8_t uid[4])
{
    if (uid == NULL || uid_queue == NULL)
    {
        return false;
    }

    if (xQueueReceive(uid_queue, uid, 0) == pdTRUE)
    {
        return true;
    }

    return false;
}
