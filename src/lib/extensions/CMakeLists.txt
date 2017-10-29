add_subdirectory(objects_introspection)
add_subdirectory(evlog)
add_subdirectory(focus_inspector)

if (${EFL_PROFILER_FOUND})
   add_subdirectory(profiling_viewer)
endif (${EFL_PROFILER_FOUND})
