FILE(GLOB_RECURSE main ${CMAKE_SOURCE_DIR}/main/*.c)

idf_component_register(SRCS ${main}
                        INCLUDE_DIRS "include" "ui"
                        REQUIRES driver bsp_extra bsp_display bsp_illuminate esp_timer
                                 esp_app_format nvs_flash)
