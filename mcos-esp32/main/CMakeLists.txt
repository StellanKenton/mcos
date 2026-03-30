idf_component_register(SRCS "main.c"
                            "system.c"
                            "app_wifi.c"
                            "bsp_wifi.c"
                            "bsp_gpio.c"
                            "DrvGpio/drvgpio.c"
                    PRIV_REQUIRES esp_wifi nvs_flash esp_driver_gpio
                    INCLUDE_DIRS "." "DrvGpio")
