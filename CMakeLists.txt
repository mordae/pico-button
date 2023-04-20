if (NOT TARGET pico_switch)
    pico_add_library(pico_switch)

    target_include_directories(pico_switch INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_sources(pico_switch INTERFACE ${CMAKE_CURRENT_LIST_DIR}/switch.c)

    target_link_libraries(pico_switch INTERFACE
                          pico_stdlib
                          pico_util)
endif()
