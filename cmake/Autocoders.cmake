function(AutocodeNetworkDefinitions)
    set(AC_SCRIPT ${FSW_ROOT}/tools/autocoders/ac_network_defs.py)
    set(INPUTS ${FSW_ROOT}/data/autocoder_inputs/network_defs.yaml)
    set(OUTPUT_DIR ${FSW_ROOT}/zephyr/include/generated/FSW)
    set(OUTPUT ${FSW_ROOT}/network_defs.h)

    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND python3 ${AC_SCRIPT} -f ${INPUTS} -o ${OUTPUT_DIR}
        DEPENDS ${AC_SCRIPT} ${INPUTS}
        COMMENT "Generating network definitions from ${INPUTS}"
    )

    target_include_directories(app PRIVATE ${OUTPUT_DIR})

    add_custom_target(autocode_network_defs DEPENDS ${OUTPUT})
endfunction()

function(AutocodeTypes)
    set(AC_SCRIPT ${CMAKE_SOURCE_DIR}/tools/autocoders/ac_types.py)
    set(INPUTS ${CMAKE_SOURCE_DIR}/data/autocoder_inputs/types.yaml)
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/zephyr/include/generated/FSW)
    set(OUTPUT ${OUTPUT_DIR}/types.h)

    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND python3 ${AC_SCRIPT} -f ${INPUTS} -o ${OUTPUT_DIR}
        DEPENDS ${AC_SCRIPT} ${INPUTS}
        COMMENT "Generating types from ${INPUTS}"
    )

    target_include_directories(app PRIVATE ${OUTPUT_DIR})

    add_custom_target(autocode_types DEPENDS ${OUTPUT})
endfunction()