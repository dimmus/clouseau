add_custom_command (
   OUTPUT clouseau_evlog.edj
   COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images ${CMAKE_CURRENT_SOURCE_DIR}/evlog.edc clouseau_evlog.edj
   DEPENDS evlog.edc
)

add_custom_target (
   clouseau_evlog_edj
   DEPENDS clouseau_evlog.edj evlog.edc
)

install (
   FILES ${CMAKE_CURRENT_BINARY_DIR}/clouseau_evlog.edj images/show-screenshot.png images/take-screenshot.png
   DESTINATION share/${PROJECT_NAME_LOWER}
)
