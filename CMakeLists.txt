cmake_minimum_required(VERSION 3.24)

set(IDF_TARGET "esp32s3")
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PRINTSPHERE_RELEASE_VERSION "v1.6-beta1" CACHE STRING "PrintSphere release version")
set(PROJECT_VER "${PRINTSPHERE_RELEASE_VERSION}")
message(STATUS "PrintSphere release version: ${PRINTSPHERE_RELEASE_VERSION}")

set(PRINTSPHERE_BOARD "onx3248g035" CACHE STRING "PrintSphere board profile")
set_property(CACHE PRINTSPHERE_BOARD PROPERTY STRINGS
    onx3248g035
)
set(PRINTSPHERE_ONX_ORIENTATION "landscape" CACHE STRING "ONX3248G035 display orientation")
set_property(CACHE PRINTSPHERE_ONX_ORIENTATION PROPERTY STRINGS
    landscape
)

if(PRINTSPHERE_BOARD STREQUAL "onx3248g035")
    if(NOT PRINTSPHERE_ONX_ORIENTATION STREQUAL "landscape")
        message(FATAL_ERROR "Unsupported PRINTSPHERE_ONX_ORIENTATION=${PRINTSPHERE_ONX_ORIENTATION}")
    endif()
    set(COMPONENTS
        main
        onx3248g035_bsp
    )
else()
    message(FATAL_ERROR "Unsupported PRINTSPHERE_BOARD=${PRINTSPHERE_BOARD}")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(PRINTSPHERE_BOARD "${PRINTSPHERE_BOARD}")
idf_build_set_property(PRINTSPHERE_ONX_ORIENTATION "${PRINTSPHERE_ONX_ORIENTATION}")
if(PRINTSPHERE_BOARD STREQUAL "onx3248g035")
    message(STATUS "PrintSphere ONX orientation: ${PRINTSPHERE_ONX_ORIENTATION}")
endif()
project(printsphere_idf)

if(IDF_VERSION_MAJOR EQUAL 6 AND IDF_VERSION_MINOR EQUAL 0 AND IDF_VERSION_PATCH EQUAL 1)
    idf_build_get_property(BUILD_COMPONENTS BUILD_COMPONENTS)
    if("espressif__esp_lvgl_adapter" IN_LIST BUILD_COMPONENTS)
        idf_component_get_property(ESP_LVGL_ADAPTER_LIB espressif__esp_lvgl_adapter COMPONENT_LIB)
        if(ESP_LVGL_ADAPTER_LIB)
            target_compile_options(${ESP_LVGL_ADAPTER_LIB} PRIVATE
                "SHELL:-include ${CMAKE_SOURCE_DIR}/main/include/printsphere/esp_timer_compat.h"
            )
        endif()
    endif()
endif()

# Work around an Xtensa GCC ICE seen in ESP-IDF 5.5.2 while compiling
# esp_lcd_panel_rgb.c with the default debug/-Og flags on Windows.
idf_component_get_property(ESP_LCD_DIR esp_lcd COMPONENT_DIR)
idf_component_get_property(ESP_LCD_LIB esp_lcd COMPONENT_LIB)
if(ESP_LCD_DIR AND ESP_LCD_LIB)
    set_source_files_properties(
        "${ESP_LCD_DIR}/rgb/esp_lcd_panel_rgb.c"
        TARGET_DIRECTORY ${ESP_LCD_LIB}
        PROPERTIES
            COMPILE_OPTIONS "-O0"
    )
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(RELEASE_INITIAL_FLASH_DIR "${CMAKE_SOURCE_DIR}/release")
set(RELEASE_INITIAL_FLASH_IMAGE "${RELEASE_INITIAL_FLASH_DIR}/firmware.bin")
set(RELEASE_INITIAL_FLASH_VERSIONED_IMAGE
    "${RELEASE_INITIAL_FLASH_DIR}/archive/firmware-${PRINTSPHERE_RELEASE_VERSION}.bin")
set(RELEASE_INITIAL_FLASH_PACKAGER "${CMAKE_SOURCE_DIR}/tools/package_initial_flash.py")
# Error lookup TSV is embedded directly into the app binary via EMBED_TXTFILES
# in main/CMakeLists.txt — no SPIFFS partition needed.

add_custom_target(
    release_initial_flash
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RELEASE_INITIAL_FLASH_DIR}"
    COMMAND ${Python3_EXECUTABLE} "${RELEASE_INITIAL_FLASH_PACKAGER}"
            --build-dir "${CMAKE_BINARY_DIR}"
            --output "${RELEASE_INITIAL_FLASH_IMAGE}"
            --version "${PRINTSPHERE_RELEASE_VERSION}"
    BYPRODUCTS
            "${RELEASE_INITIAL_FLASH_IMAGE}"
            "${RELEASE_INITIAL_FLASH_VERSIONED_IMAGE}"
    DEPENDS
            app
            bootloader
            "${CMAKE_BINARY_DIR}/flasher_args.json"
            "${RELEASE_INITIAL_FLASH_PACKAGER}"
    COMMENT "Packaging initial flash image at ${RELEASE_INITIAL_FLASH_IMAGE}"
    VERBATIM
)
