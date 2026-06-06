idf_build_get_property(PRINTSPHERE_BOARD PRINTSPHERE_BOARD)
idf_build_get_property(PRINTSPHERE_ONX_ORIENTATION PRINTSPHERE_ONX_ORIENTATION)

set(PRINTSPHERE_BOARD_REQUIRES)
set(PRINTSPHERE_BOARD_DEFINES)

if(PRINTSPHERE_BOARD STREQUAL "onx3248g035")
    list(APPEND PRINTSPHERE_BOARD_REQUIRES onx3248g035_bsp)
    list(APPEND PRINTSPHERE_BOARD_DEFINES PRINTSPHERE_BOARD_ONX3248G035=1)
    if(PRINTSPHERE_ONX_ORIENTATION STREQUAL "landscape")
        list(APPEND PRINTSPHERE_BOARD_DEFINES PRINTSPHERE_ONX_ORIENTATION_LANDSCAPE=1)
    else()
        message(FATAL_ERROR "Unsupported PRINTSPHERE_ONX_ORIENTATION=${PRINTSPHERE_ONX_ORIENTATION}")
    endif()
else()
    message(FATAL_ERROR "Unsupported PRINTSPHERE_BOARD=${PRINTSPHERE_BOARD}")
endif()

message(STATUS "PrintSphere board profile: ${PRINTSPHERE_BOARD}")
if(PRINTSPHERE_BOARD STREQUAL "onx3248g035")
    message(STATUS "PrintSphere ONX orientation: ${PRINTSPHERE_ONX_ORIENTATION}")
endif()

idf_component_register(
    SRCS
        "app_main.cpp"
        "src/application.cpp"
        "src/bambu_status.cpp"
        "src/bambu_cloud_client.cpp"
        "src/config_store.cpp"
        "src/error_lookup.cpp"
        "include/font/dosis_20.c"
        "include/font/dosis_32.c"
        "include/font/dosis_40.c"
        "include/font/onx_cjk_16.c"
        "include/img/bambuicon.c"
        "include/font/mdi_30.c"
        "include/font/mdi_40.c"
        "src/pmu.cpp"
        "src/p1s_camera_client.cpp"
        "src/printer_client.cpp"
        "src/printer_state.cpp"
        "src/debug_log_buffer.cpp"
        "src/setup_portal.cpp"
        "src/status_resolver.cpp"
        "src/time_sync.cpp"
        "src/ui.cpp"
        "src/wifi_manager.cpp"
    INCLUDE_DIRS
        "include"
    EMBED_TXTFILES
        "include/certs/bambu.cert"
        "include/certs/bambu_p2s_250626.cert"
        "include/certs/bambu_h2c_251122.cert"
        "include/certs/globalsign_root_r3.pem"
        "include/error_lookup/error_lookup.tsv"
    REQUIRES
        ${PRINTSPHERE_BOARD_REQUIRES}
        app_update
        esp_https_ota
        esp_driver_gpio
        esp_event
        esp_http_client
        esp_http_server
        mqtt
        esp_netif
        lwip
        espressif__esp_new_jpeg
        esp-tls
        esp_wifi
        libpng
        lvgl
        nvs_flash
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    PRINTSPHERE_RELEASE_VERSION="${PRINTSPHERE_RELEASE_VERSION}"
    ${PRINTSPHERE_BOARD_DEFINES}
)
string(FIND "${PRINTSPHERE_RELEASE_VERSION}" "-debug" _debug_pos)
string(FIND "${PRINTSPHERE_RELEASE_VERSION}" "_debug" _debug_pos2)
if(_debug_pos GREATER -1 OR _debug_pos2 GREATER -1)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE PRINTSPHERE_DEBUG_BUILD=1)
endif()
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra)
