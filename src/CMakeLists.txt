mulog_test_register_test(list)
set_target_properties(list_test PROPERTIES CXX_STANDARD 20)

if (NOT MULOG_ENABLE_DEFERRED_LOGGING)
    mulog_test_register_test(mulog_realtime mulog)
    set_target_properties(mulog_realtime_test PROPERTIES CXX_STANDARD 20)
    target_compile_definitions(mulog_realtime_test PRIVATE
            -DMULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_TIMESTAMP_OUTPUT}>,1,0>
            -DMULOG_INTERNAL_ENABLE_COLOR_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_COLOR_OUTPUT}>,1,0>)
    target_include_directories(mulog_realtime_test PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    mulog_add_coverage_flags(mulog_realtime_test)

    mulog_test_register_test(mulog_realtime_lock mulog)
    set_target_properties(mulog_realtime_lock_test PROPERTIES CXX_STANDARD 20)
    target_compile_definitions(mulog_realtime_lock_test PRIVATE
            -DMULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_TIMESTAMP_OUTPUT}>,1,0>
            -DMULOG_INTERNAL_ENABLE_COLOR_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_COLOR_OUTPUT}>,1,0>)
    mulog_test_add_wrappers(mulog_realtime_lock vsnprintf_ snprintf_)
    mulog_add_coverage_flags(mulog_realtime_lock_test)
else ()
    mulog_test_register_test(mulog_deferred mulog)
    set_target_properties(mulog_deferred_test PROPERTIES CXX_STANDARD 20)
    target_compile_definitions(mulog_deferred_test PRIVATE
            -DMULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_TIMESTAMP_OUTPUT}>,1,0>
            -DMULOG_INTERNAL_ENABLE_COLOR_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_COLOR_OUTPUT}>,1,0>)
    target_include_directories(mulog_deferred_test PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    mulog_add_coverage_flags(mulog_deferred_test)

    mulog_test_register_test(mulog_deferred_lock mulog)
    set_target_properties(mulog_deferred_lock_test PROPERTIES CXX_STANDARD 20)
    target_compile_definitions(mulog_deferred_lock_test PRIVATE
            -DMULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_TIMESTAMP_OUTPUT}>,1,0>
            -DMULOG_INTERNAL_ENABLE_COLOR_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_COLOR_OUTPUT}>,1,0>)
    mulog_test_add_wrappers(mulog_deferred_lock vsnprintf_ snprintf_ lwrb_get_full lwrb_get_linear_block_read_length)
    target_link_libraries(mulog_deferred_lock_test PRIVATE lwrb)
    mulog_add_coverage_flags(mulog_deferred_lock_test)
endif ()
