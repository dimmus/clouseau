add_custom_command (
   OUTPUT clouseau_evlog.edj
   COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images ${CMAKE_CURRENT_SOURCE_DIR}/evlog.edc clouseau_evlog.edj
   DEPENDS evlog.edc
)
add_custom_command (
   OUTPUT clouseau_focus_inspector.edj
   COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images ${CMAKE_CURRENT_SOURCE_DIR}/focus_inspector.edc clouseau_focus_inspector.edj
   DEPENDS focus_inspector.edc
)

add_custom_target (
   clouseau_evlog_edj
   DEPENDS clouseau_evlog.edj evlog.edc
)

add_custom_target(focus_inspector_theme DEPENDS clouseau_focus_inspector.edj)

install (
   FILES ${CMAKE_CURRENT_BINARY_DIR}/clouseau_focus_inspector.edj ${CMAKE_CURRENT_BINARY_DIR}/clouseau_evlog.edj images/show-screenshot.png images/take-screenshot.png
   DESTINATION share/${PROJECT_NAME_LOWER}
)
