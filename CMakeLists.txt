cmake_minimum_required(VERSION 2.8.1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

project(CLOUSEAU)

#set(EA_GEN_COMMON_DEFINITIONS -DEFL_EO_API_SUPPORT)
#add_definitions(${EA_GEN_COMMON_DEFINITIONS})

find_package(Efl REQUIRED)
find_package(Ffi REQUIRED)
find_package(Eina REQUIRED)
find_package(Eet REQUIRED)
find_package(Eolian REQUIRED)
find_package(Eo REQUIRED)
find_package(Elementary REQUIRED)
find_package(Ecore REQUIRED)
find_package(Evas REQUIRED)

add_subdirectory(src/bin)
