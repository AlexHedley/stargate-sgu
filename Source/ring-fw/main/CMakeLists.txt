idf_component_register(SRCS "GPIO.c" "gpio.c" "main.c"
    INCLUDE_DIRS "."
    REQUIRES lwip log esp_common esp_app_format esp_event esp_wifi esp_hw_support nvs_flash esp_pm driver SGU app_update led_strip)
