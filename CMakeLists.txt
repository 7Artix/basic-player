cmake_minimum_required(VERSION 3.16)

project(basic-player CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/app/*.cpp
    ${CMAKE_SOURCE_DIR}/source/**/*.cpp
)

add_executable(basic-player ${SOURCES})

target_include_directories(basic-player PRIVATE
    ${CMAKE_SOURCE_DIR}/app/
    ${CMAKE_SOURCE_DIR}/source/
    ${CMAKE_SOURCE_DIR}/source/common
    ${CMAKE_SOURCE_DIR}/source/controller
    ${CMAKE_SOURCE_DIR}/source/decoder
    ${CMAKE_SOURCE_DIR}/source/demuxer
    ${CMAKE_SOURCE_DIR}/source/displayer
    ${CMAKE_SOURCE_DIR}/source/filter
    ${CMAKE_SOURCE_DIR}/source/loader
    ${CMAKE_SOURCE_DIR}/source/player
    ${CMAKE_SOURCE_DIR}/source/renderer
    ${CMAKE_SOURCE_DIR}/source/tools
    ${CMAKE_SOURCE_DIR}/source/drivers
    ${CMAKE_SOURCE_DIR}/source/drivers/tft/ST7735S
    ${CMAKE_SOURCE_DIR}/source/drivers/oled/SSD1306
)

target_link_libraries(basic-player
    gpiodcxx
    gpiod
    avformat
    avcodec
    avutil
    swscale
    avfilter
    pthread
)