idf_component_register(SRCS "main.c"
                            "system.c"
                            "app_wifi.c"
                            "bsp_wifi.c"
                    PRIV_REQUIRES esp_wifi nvs_flash
                    INCLUDE_DIRS ".")
