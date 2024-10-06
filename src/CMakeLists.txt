cpputest_register_test(list)
set_target_properties(list_test PROPERTIES CXX_STANDARD 20)

if (NOT MULOG_ENABLE_DEFERRED_LOGGING)
    cpputest_register_test(mulog_realtime mulog)
    set_target_properties(mulog_realtime_test PROPERTIES CXX_STANDARD 20)
    target_compile_definitions(mulog_realtime_test PRIVATE
            -DMULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_TIMESTAMP_OUTPUT}>,1,0>
            -DMULOG_INTERNAL_ENABLE_COLOR_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_COLOR_OUTPUT}>,1,0>)
    target_include_directories(mulog_realtime_test PRIVATE ${CMAKE_CURRENT_LIST_DIR})
else ()
    cpputest_register_test(mulog_deferred mulog)
    set_target_properties(mulog_deferred_test PROPERTIES CXX_STANDARD 20)
    target_compile_definitions(mulog_deferred_test PRIVATE
            -DMULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_TIMESTAMP_OUTPUT}>,1,0>
            -DMULOG_INTERNAL_ENABLE_COLOR_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_COLOR_OUTPUT}>,1,0>)
    target_include_directories(mulog_deferred_test PRIVATE ${CMAKE_CURRENT_LIST_DIR})
endif ()
