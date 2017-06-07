set(CMAKE_BUILD_TYPE Debug)

add_definitions(-DEFL_EO_API_SUPPORT -DEVLOG_EDJ="${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME_LOWER}/clouseau_evlog.edj")

set(EOLIAN_INCLUDE_FLAG -I \"${EOLIAN_EO_DIR}/eo-1\" -I${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
   ${CMAKE_CURRENT_BINARY_DIR}
   ${ELEMENTARY_INCLUDE_DIRS}
   ${EVAS_INCLUDE_DIRS}
   ${EO_INCLUDE_DIRS}
   ${EINA_INCLUDE_DIRS}
   ${EET_INCLUDE_DIRS}
   )

link_directories (${PC_LIBELEMENTARY_LIBRARY_DIRS})

add_library(clouseau_evlog SHARED main.c)

target_link_libraries(clouseau_evlog
   ${ELEMENTARY_LIBRARIES}
   ${EVAS_LIBRARIES}
   ${EO_LIBRARIES}
   ${EINA_LIBRARIES}
   ${EET_LIBRARIES}
   )

INSTALL(TARGETS clouseau_evlog LIBRARY DESTINATION lib)

