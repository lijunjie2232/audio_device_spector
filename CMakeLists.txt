cmake_minimum_required(VERSION 3.20)

project(AudioOutputMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(audio_output_monitor
    src/main.cpp
    src/app.cpp
    src/engine.cpp
    src/capture.cpp
)

target_include_directories(audio_output_monitor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(audio_output_monitor PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)

if(MSVC)
    target_compile_options(audio_output_monitor PRIVATE /utf-8 /EHsc /W3)
    target_compile_options(audio_output_monitor PRIVATE $<$<CONFIG:Release>:/O2>)
endif()

target_link_libraries(audio_output_monitor PRIVATE
    user32
    gdi32
    ole32
    d2d1
    dwrite
    dwmapi
)

set_target_properties(audio_output_monitor PROPERTIES
    WIN32_EXECUTABLE TRUE
)
