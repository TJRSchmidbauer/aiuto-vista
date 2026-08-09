if(CONFIG_CYD_MCP4725_SUPPORT)
    set(component_sources bsp_extra_dac.c)
else()
    set(component_sources bsp_extra.c)
endif()

idf_component_register(SRCS ${component_sources}
                        INCLUDE_DIRS "include"
                        REQUIRES driver esp_driver_rmt esp_driver_i2c esp_timer freertos)