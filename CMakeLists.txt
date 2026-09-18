cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME BASE_PROJECT)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: ${CMAKE_BUILD_TYPE}")

# Enable CMake support for ASM and C languages
enable_language(C ASM)

set(ARM_CPU_FLAGS
    -mcpu=cortex-m3
    -mthumb
)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Keep user code in a separate target so CubeMX regeneration stays clean.
add_library(User_main OBJECT)
target_sources(User_main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/src/user_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/src/led_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/src/key_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/src/adc_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/src/buzzer_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/src/pulse_capture.c
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/src/cdc_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/src/font_8x16.c
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/src/oled/lcd_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/src/oled/page_home.c
)
target_include_directories(User_main PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/src/oled
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/Target
)
target_compile_options(User_main PRIVATE
    ${ARM_CPU_FLAGS}
    -ffunction-sections
    -fdata-sections
)
target_link_libraries(User_main PUBLIC stm32cubemx)

include(cmake/user.cmake)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/User_APP/src/oled
)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    ${ARM_CPU_FLAGS}
    -ffunction-sections
    -fdata-sections
)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    "LINKER:-Map=${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map"
    "LINKER:--cref"
    "LINKER:--print-memory-usage"
    "LINKER:--gc-sections"
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    User_main
    m
)
