cmake_minimum_required(VERSION 3.5)

set(IDF_TARGET "esp32s3" CACHE STRING "ESP-IDF target")

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/esp-idf/components"
    "${CMAKE_CURRENT_LIST_DIR}/esp-idf/src/components"
    "${CMAKE_CURRENT_LIST_DIR}/esp-idf/src/main"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wake_word_detection)
