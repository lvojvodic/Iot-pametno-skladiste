
#include "card_storage.h"

#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

// Baza registrovanih kartica
typedef struct
{
    uint8_t count;
    uint8_t uid[MAX_CARDS][4];
} card_database_t;

static card_database_t database;
static const char *TAG = "CARDS";

// Cuvanje kartica u NVS
static esp_err_t cards_save(void)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(
        "access", NVS_READWRITE, &handle
    );

    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_set_blob(
        handle, "cards", &database, sizeof(database)
    );

    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    return err;
}

// Inicijalizacija i ucitavanje kartica
esp_err_t cards_init(void)
{
    esp_err_t err = nvs_flash_init();

    if (err != ESP_OK)
    {
        return err;
    }

    nvs_handle_t handle;

    err = nvs_open(
        "access", NVS_READWRITE, &handle
    );

    if (err != ESP_OK)
    {
        return err;
    }

    size_t size = sizeof(database);

    memset(&database, 0, sizeof(database));

    err = nvs_get_blob(
        handle, "cards", &database, &size
    );

    nvs_close(handle);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        return ESP_OK;
    }

    if (err != ESP_OK)
    {
        return err;
    }

    if (size != sizeof(database) ||
        database.count > MAX_CARDS)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

// Provjera da li je kartica registrovana
bool cards_contains(const uint8_t uid[4])
{
    for (int i = 0; i < database.count; i++)
    {
        if (memcmp(database.uid[i], uid, 4) == 0)
        {
            return true;
        }
    }

    return false;
}

// Registracija nove kartice
esp_err_t cards_add(const uint8_t uid[4])
{
    if (cards_contains(uid))
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (database.count >= MAX_CARDS)
    {
        return ESP_ERR_NO_MEM;
    }

    card_database_t old = database;

    memcpy(database.uid[database.count], uid, 4);
    database.count++;

    esp_err_t err = cards_save();

    if (err != ESP_OK)
    {
        database = old;
    }

    return err;
}

// Brisanje registrovane kartice
esp_err_t cards_remove(const uint8_t uid[4])
{
    int position = -1;

    for (int i = 0; i < database.count; i++)
    {
        if (memcmp(database.uid[i], uid, 4) == 0)
        {
            position = i;
            break;
        }
    }

    if (position == -1)
    {
        return ESP_ERR_NOT_FOUND;
    }

    card_database_t old = database;

    for (int i = position; i < database.count - 1; i++)
    {
        memcpy(database.uid[i], database.uid[i + 1], 4);
    }

    database.count--;

    esp_err_t err = cards_save();

    if (err != ESP_OK)
    {
        database = old;
    }

    return err;
}

// Prikaz registrovanih kartica
void cards_list(void)
{
    ESP_LOGI(
        TAG,
        "Registrovanih kartica: %u",
        (unsigned)database.count
    );

    for (int i = 0; i < database.count; i++)
    {
        ESP_LOGI(
            TAG,
            "Kartica %d: %02X:%02X:%02X:%02X",
            i + 1,
            database.uid[i][0],
            database.uid[i][1],
            database.uid[i][2],
            database.uid[i][3]
        );
    }
}
