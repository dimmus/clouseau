set(CMAKE_BUILD_TYPE Debug)

add_definitions(${CLI_COMMON_DEFINITIONS} -DEFL_EO_API_SUPPORT)

add_definitions(-DEFL_EO_API_SUPPORT -DGUI_IMAGES_PATH="${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME_LOWER}")

include_directories(
   ${CMAKE_CURRENT_BINARY_DIR}
   ${ELEMENTARY_INCLUDE_DIRS}
   ${EVAS_INCLUDE_DIRS}
   ${EO_INCLUDE_DIRS}
   ${EINA_INCLUDE_DIRS}
   ${EET_INCLUDE_DIRS}
   ${EOLIAN_INCLUDE_DIRS}
   )

link_directories (${PC_LIBELEMENTARY_LIBRARY_DIRS})

add_library(clouseau_objects_introspection SHARED main.c gui.c)

target_link_libraries(clouseau_objects_introspection
   ${ELEMENTARY_LIBRARIES}
   ${EVAS_LIBRARIES}
   ${EO_LIBRARIES}
   ${EINA_LIBRARIES}
   ${EET_LIBRARIES}
   ${EOLIAN_LIBRARIES}
   clouseau_debug
   )

INSTALL(TARGETS clouseau_objects_introspection LIBRARY DESTINATION lib)
