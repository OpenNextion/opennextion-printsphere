set(ONX_BSP_SRCS
    "src/onx3248g035_bsp.c"
    "src/onx3248g035_bsp_compat.c"
)

set(ONX_BSP_REQUIRES
    esp_driver_gpio
    esp_driver_i2c
    esp_driver_ledc
    esp_driver_spi
    esp_adc
    esp_lcd
    esp_timer
)

idf_build_get_property(PRINTSPHERE_ONX_ORIENTATION PRINTSPHERE_ONX_ORIENTATION)
if(NOT PRINTSPHERE_ONX_ORIENTATION)
    set(PRINTSPHERE_ONX_ORIENTATION "portrait")
endif()
set(ONX_BSP_ORIENTATION_DEFINES PRINTSPHERE_ONX_ORIENTATION_PORTRAIT=1)
if(PRINTSPHERE_ONX_ORIENTATION STREQUAL "landscape")
    set(ONX_BSP_ORIENTATION_DEFINES PRINTSPHERE_ONX_ORIENTATION_LANDSCAPE=1)
elseif(NOT PRINTSPHERE_ONX_ORIENTATION STREQUAL "portrait")
    message(FATAL_ERROR "Unsupported PRINTSPHERE_ONX_ORIENTATION=${PRINTSPHERE_ONX_ORIENTATION}")
endif()

set(ONX_BSP_HAS_LVGL 0)
set(ONX_BSP_LVGL_COMPONENT "")
idf_build_get_property(build_components BUILD_COMPONENTS)
if("lvgl" IN_LIST build_components)
    set(ONX_BSP_HAS_LVGL 1)
    set(ONX_BSP_LVGL_COMPONENT lvgl)
    list(APPEND ONX_BSP_SRCS "src/onx3248g035_bsp_lvgl.c")
    list(APPEND ONX_BSP_REQUIRES lvgl)
elseif("lvgl__lvgl" IN_LIST build_components)
    set(ONX_BSP_HAS_LVGL 1)
    set(ONX_BSP_LVGL_COMPONENT lvgl__lvgl)
    list(APPEND ONX_BSP_SRCS "src/onx3248g035_bsp_lvgl.c")
    list(APPEND ONX_BSP_REQUIRES lvgl__lvgl)
endif()

idf_component_register(
    SRCS
        ${ONX_BSP_SRCS}
    INCLUDE_DIRS
        "include"
    REQUIRES
        ${ONX_BSP_REQUIRES}
    WHOLE_ARCHIVE
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    ${ONX_BSP_ORIENTATION_DEFINES}
)

if(ONX_BSP_HAS_LVGL)
    idf_component_get_property(lvgl_component_dir ${ONX_BSP_LVGL_COMPONENT} COMPONENT_DIR)
    target_include_directories(${COMPONENT_LIB} PRIVATE
        "${lvgl_component_dir}"
        "${lvgl_component_dir}/src"
        "${lvgl_component_dir}/.."
    )
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        ONX_BSP_HAS_LVGL=1
        LV_CONF_INCLUDE_SIMPLE
    )
endif()
