FILE(GLOB_RECURSE component_sources "*.c")

idf_component_register(SRCS ${component_sources}
                        INCLUDE_DIRS "include"
                        REQUIRES nvs_flash esp_lcd esp_lcd_touch_xpt2046 esp_lcd_ili9341)
