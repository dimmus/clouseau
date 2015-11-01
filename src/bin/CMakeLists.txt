set(CMAKE_BUILD_TYPE Debug)

LIST(APPEND SOURCES clouseau_client.c gui.c)


add_definitions(${CLI_COMMON_DEFINITIONS} -DEFL_BETA_API_SUPPORT)
add_definitions(${CLI_COMMON_DEFINITIONS})

include_directories(
   ${ELEMENTARY_INCLUDE_DIRS}
   ${EINA_INCLUDE_DIRS}
   ${EET_INCLUDE_DIRS}
   ${EO_INCLUDE_DIRS}
   ${EOLIAN_INCLUDE_DIRS}
   ${ECORE_INCLUDE_DIRS}
   ${EVAS_INCLUDE_DIRS}
   )

set(EOLIAN_INCLUDE_FLAG -I \"${EOLIAN_EO_DIR}/eo-1\" -I${CMAKE_CURRENT_SOURCE_DIR})

add_executable(clouseau_client ${SOURCES})

target_link_libraries(clouseau_client
   pthread
   ${EFL_LIBRARIES}
   ${ELEMENTARY_LIBRARIES}
   ${EINA_LIBRARIES}
   ${EET_LIBRARIES}
   ${EO_LIBRARIES}
   ${EOLIAN_LIBRARIES}
   ${ECORE_LIBRARIES}
   ${EVAS_LIBRARIES}
   )

