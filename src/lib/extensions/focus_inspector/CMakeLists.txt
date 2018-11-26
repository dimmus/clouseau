set(CMAKE_BUILD_TYPE Debug)

add_definitions(${CLI_COMMON_DEFINITIONS} -DEFL_EO_API_SUPPORT
   -DFOCUS_EDJ="${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME_LOWER}/clouseau_focus_inspector.edj")

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

add_library(clouseau_focus_inspector SHARED main.c gui.c tree_view.c)

target_link_libraries(clouseau_focus_inspector
   ${ELEMENTARY_LIBRARIES}
   ${EVAS_LIBRARIES}
   ${EO_LIBRARIES}
   ${EINA_LIBRARIES}
   ${EET_LIBRARIES}
   ${EOLIAN_LIBRARIES}
   clouseau_debug
   )

add_dependencies(clouseau_focus_inspector focus_inspector_theme)

INSTALL(TARGETS clouseau_focus_inspector LIBRARY DESTINATION lib)
