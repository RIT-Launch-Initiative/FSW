function(AutocodeNetworkDefinitions)
    set(AC_SCRIPT ${ZEPHYR_FSW_MODULE_DIR}/tools/autocoders/ac_network_defs.py)
    set(DEFAULT_INPUTS ${ZEPHYR_FSW_MODULE_DIR}/data/autocoder_inputs/network_defs.yaml)
    set(INPUTS ${DEFAULT_INPUTS})
    if(ARGC GREATER 0)
        foreach(INPUT ${ARGV})
            list(APPEND INPUTS ${INPUT})
        endforeach()
    endif()
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/zephyr/include/generated/FSW)
    set(OUTPUT ${OUTPUT_DIR}/n_autocoder_network_defs.h)

    file(MAKE_DIRECTORY ${OUTPUT_DIR})

    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND python3 ${AC_SCRIPT} -f ${INPUTS} -o ${OUTPUT}
        DEPENDS ${AC_SCRIPT} ${INPUTS}
        COMMENT "Generating network definitions from ${INPUTS}"
    )

    target_include_directories(app PRIVATE ${OUTPUT_DIR})

    add_custom_target(ac_net_defs DEPENDS ${OUTPUT})
endfunction()

function(AutocodeTypes)
    set(AC_SCRIPT ${ZEPHYR_FSW_MODULE_DIR}/tools/autocoders/ac_types.py)
    set(DEFAULT_INPUTS ${ZEPHYR_FSW_MODULE_DIR}/data/autocoder_inputs/types.yaml)
    set(INPUTS ${DEFAULT_INPUTS})
    if(ARGC GREATER 0)
        foreach(INPUT ${ARGV})
            list(APPEND INPUTS ${INPUT})
        endforeach()
    endif()
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/zephyr/include/generated/FSW)
    set(OUTPUT ${OUTPUT_DIR}/n_autocoder_types.h)

    file(MAKE_DIRECTORY ${OUTPUT_DIR})

    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND python3 ${AC_SCRIPT} -f ${INPUTS} -o ${OUTPUT}
        DEPENDS ${AC_SCRIPT} ${INPUTS}
        COMMENT "Generating types from ${INPUTS}"
    )

    target_include_directories(app PRIVATE ${OUTPUT_DIR})

    add_custom_target(ac_types DEPENDS ${OUTPUT})
endfunction()
