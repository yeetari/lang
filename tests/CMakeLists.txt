add_custom_target(tests ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.bash $<TARGET_FILE:kodoc>)
add_dependencies(tests kodoc)
