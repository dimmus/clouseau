set(CMAKE_BUILD_TYPE Debug)

include_directories(
   ${CMAKE_CURRENT_BINARY_DIR}
   ${EINA_INCLUDE_DIRS}
   ${EOLIAN_INCLUDE_DIRS}
   ${EO_INCLUDE_DIRS}
   ${FFI_INCLUDE_DIRS}
   ${EVAS_INCLUDE_DIRS}
   ${ELEMENTARY_INCLUDE_DIRS}
   )

add_library(clouseau_debug SHARED clouseau_debug.c)

target_link_libraries(clouseau_debug
   ${EINA_LIBRARIES}
   ${EO_LIBRARIES}
   ${EOLIAN_LIBRARIES}
   ${FFI_LIBRARIES}
   )

INSTALL(FILES Clouseau.h DESTINATION include)

INSTALL(TARGETS clouseau_debug LIBRARY DESTINATION lib)

add_subdirectory(extensions)
