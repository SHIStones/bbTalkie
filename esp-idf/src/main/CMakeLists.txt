set(requires
    hardware_driver
    player
    esp_wifi
    esp_timer
    nvs_flash
    )

idf_component_register(SRCS
                           main.c
                           app_state.c
                           bb_animation.c
                           bb_audio.c
                           bb_button.c
                           bb_display.c
                           bb_led.c
                           bb_power.c
                           bb_radio.c
                       REQUIRES ${requires}
                       PRIV_REQUIRES esp32-spi-ssd1327
                       )

