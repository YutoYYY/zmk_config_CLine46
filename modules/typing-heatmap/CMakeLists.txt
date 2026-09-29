# キー使用回数の記録(cormoran/zmk-feature-typing-heatmap を CLine46 向けに改造したもの)
zephyr_include_directories(include)
if(CONFIG_ZMK_FEATURE_TYPING_HEATMAP AND ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
    # Observe positions before combo/keymap listeners can capture them.
    target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/typing_heatmap.c
                               ${CMAKE_CURRENT_SOURCE_DIR}/src/typing_heatmap_state.c)
    # Observe how the keymap resolves each press (layer and held modifiers).
    zephyr_ld_options(-Wl,--wrap=zmk_behavior_invoke_binding)

    if(CONFIG_ZMK_FEATURE_TYPING_HEATMAP_STUDIO_RPC)
        target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/studio/feature_typing_heatmap_handler.c)

        list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
        include(nanopb)
        set(NANOPB_GENERATE_CPP_APPEND_PATH TRUE)
        set(NANOPB_GENERATE_CPP_STANDALONE OFF)

        # NOTE: adding to app target directly causes build issues, so we create a library instead
        zephyr_library_named(cline46_typing_heatmap_proto)
        file(GLOB_RECURSE PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

        nanopb_generate_cpp(proto_srcs proto_hdrs RELPATH ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILES})
        target_include_directories(${ZEPHYR_CURRENT_LIBRARY} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
        target_sources(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${proto_srcs} ${proto_hdrs})

        # app should depend on the generated proto files
        target_include_directories(app PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/proto)
        add_dependencies(app ${ZEPHYR_CURRENT_LIBRARY})
    endif()
endif()
