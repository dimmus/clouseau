set(CMAKE_BUILD_TYPE Debug)

LIST(APPEND SOURCES clouseau_client.c gui.c)

add_definitions(-DEFL_EO_API_SUPPORT -DGUI_IMAGES_PATH="${CMAKE_INSTALL_PREFIX}/data/images")
add_definitions(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

link_directories (${PC_LIBELEMENTARY_LIBRARY_DIRS})

include_directories(
   ${CMAKE_SOURCE_DIR}/src/lib
   ${ELEMENTARY_INCLUDE_DIRS}
   ${EINA_INCLUDE_DIRS}
   ${EET_INCLUDE_DIRS}
   ${EFREET_INCLUDE_DIRS}
   ${EO_INCLUDE_DIRS}
   ${ECORE_INCLUDE_DIRS}
   ${EVAS_INCLUDE_DIRS}
   )

set(EOLIAN_INCLUDE_FLAG -I \"${EOLIAN_EO_DIR}/eo-1\" -I${CMAKE_CURRENT_SOURCE_DIR})

add_executable(clouseau_client ${SOURCES})

target_link_libraries(clouseau_client
   pthread
   clouseau_debug
   ${EFL_LIBRARIES}
   ${ELEMENTARY_LIBRARIES}
   ${EINA_LIBRARIES}
   ${EET_LIBRARIES}
   ${EFREET_LIBRARIES}
   ${EO_LIBRARIES}
   ${ECORE_LIBRARIES}
   ${EVAS_LIBRARIES}
   ecore_file
   )

install(TARGETS clouseau_client DESTINATION bin)
install(PROGRAMS clouseau DESTINATION bin)
