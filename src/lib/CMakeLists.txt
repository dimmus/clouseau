set(CMAKE_BUILD_TYPE Debug)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(
   ${EINA_INCLUDE_DIRS}
   ${EO_INCLUDE_DIRS}
   )

add_library(clouseau_debug SHARED clouseau_debug.c)

target_link_libraries(clouseau_debug
   ${EINA_LIBRARIES}
   ${EO_LIBRARIES}
   )

INSTALL(TARGETS clouseau_debug LIBRARY DESTINATION lib)
