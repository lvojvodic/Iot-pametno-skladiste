
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        pir
        buzzer
        rfid
        lcd
        rgb
        card_storage
        esp_driver_uart
        esp_timer
)







