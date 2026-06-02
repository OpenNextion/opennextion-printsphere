cmake_minimum_required(VERSION 3.24)

set(IDF_TARGET "esp32s3")
set(EXTRA_COMPONENT_DIRS
    "../../components/onx3248g035_bsp"
    "../../managed_components/lvgl__lvgl"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(onx_bsp_lvgl_smoke)
