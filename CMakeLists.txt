cmake_minimum_required(VERSION 2.8.1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

project(CLOUSEAU)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

set(DEBUG_COMMON_DEFINITIONS -DEFL_EO_API_SUPPORT -DEFL_BETA_API_SUPPORT)
add_definitions(${DEBUG_COMMON_DEFINITIONS})

find_package(Efl REQUIRED)
find_package(Eina REQUIRED)
find_package(Eet REQUIRED)
find_package(Efreet REQUIRED)
find_package(Eolian REQUIRED)
find_package(Eo REQUIRED)
find_package(Elementary REQUIRED)
find_package(Ecore REQUIRED)
find_package(Evas REQUIRED)
find_package(Edebug REQUIRED)

add_subdirectory(src/bin)
add_subdirectory(src/lib)
add_subdirectory(data)

add_dependencies(clouseau_evlog clouseau_evlog_edj)
