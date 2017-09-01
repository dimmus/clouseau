add_subdirectory(objects_introspection)
add_subdirectory(evlog)

if (${EFL_PROFILER_FOUND})
   add_subdirectory(profiling_viewer)
endif (${EFL_PROFILER_FOUND})
