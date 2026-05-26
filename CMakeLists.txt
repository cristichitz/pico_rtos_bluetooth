# C7222 development workspace entrypoint.

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# ==========================================================================
# define the PICO board as Pico 2 W variant.
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# define the path for the ELEC-C7222 SDK. This is required for the project to build.
set(_DEFAULT_ELEC_C7222_API_PATH "${CMAKE_CURRENT_LIST_DIR}/../c7222-development")
set(ELEC_C7222_API_PATH "${_DEFAULT_ELEC_C7222_API_PATH}" CACHE PATH
    "Path to the ELEC-C7222 SDK. This is required for the project to build.")

if(DEFINED ENV{ELEC_C7222_API_PATH} AND NOT "$ENV{ELEC_C7222_API_PATH}" STREQUAL "")
    if(EXISTS "$ENV{ELEC_C7222_API_PATH}")
        set(ELEC_C7222_API_PATH "$ENV{ELEC_C7222_API_PATH}" CACHE PATH
            "Path to the ELEC-C7222 SDK. This is required for the project to build."
            FORCE)
        message(STATUS "Using ELEC_C7222_API_PATH from environment: ${ELEC_C7222_API_PATH}")
    else()
        message(WARNING
            "ELEC_C7222_API_PATH is set but does not exist: $ENV{ELEC_C7222_API_PATH}")
    endif()
endif()

# set the project name
set(PROJECT_NAME "ELEC-C7222-Task" CACHE STRING "Project name")
# define the options for the project
option(C7222_ENABLE_BLE "Enable BLE helpers in ELEC_C7222" ON)
option(C7222_BLE_DEBUG "Enable BLE debug logging" ON)
option(C7222_EXAMPLES_BUILD "Enable C7222 examples build" OFF)
option(C7222_GETTING_STARTED_BUILD "Enable getting-started build targets" OFF)
option(C7222_EXPORT_PICO_UF2 "Export UF2 image copy in build/images after build" OFF)

# ====================================================================================
# Centralized toolchain + SDK + FreeRTOS setup.
include(${ELEC_C7222_API_PATH}/cmake/c7222_development.cmake)
c7222_prepare_pre_project()

# ====================================================================================
# PROJECT
project(${PROJECT_NAME} C CXX ASM)

# ====================================================================================
# Project component setup that depends on project()/pico_sdk_init().
c7222_prepare_post_project()


# create the application interface library

set(APP_NAME ${PROJECT_NAME})
# if we are not building examples, just build the app from the main source dir
set(APP_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
file(GLOB APP_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.c")
file(GLOB APP_GATT_FILES
    "${CMAKE_SOURCE_DIR}/src/*.gatt")

# Enforce stricter warnings for user sources in pico-project/src only.
set_source_files_properties(${APP_SOURCES}
    PROPERTIES COMPILE_OPTIONS "-Wall;-Wpedantic")

add_library(APP_LIB INTERFACE)
target_sources(APP_LIB INTERFACE
                ${APP_SOURCES})

target_include_directories(APP_LIB INTERFACE
                ${APP_SOURCE_DIR})

set_property(TARGET APP_LIB PROPERTY TARGET_NAME ${APP_NAME})
if(APP_GATT_FILES)
    set_property(TARGET APP_LIB PROPERTY GATT_FILES "${APP_GATT_FILES}")
endif()

c7222_configure_app_target(${APP_NAME} APP_LIB)
