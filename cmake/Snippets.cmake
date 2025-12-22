function(AddSnippets)
    SET(snippets ${ARGN})

    foreach(snippet ${snippets})
        list(FIND SNIPPET ${snippet} snippet_index)

        if(snippet_index EQUAL -1)
            list(APPEND SNIPPET ${snippet})
        endif()
    endforeach()

    set(SNIPPET ${SNIPPET} CACHE STRING "List of snippets" FORCE)
endfunction()

function(AddCommonSnippets)
    list(APPEND COMMON_SNIPPETS
            "bpl-networking"
            "cpp"
            "lfs"
    )

    AddSnippets(${COMMON_SNIPPETS})
endfunction()

function(AddDebugSnippets)
    list(APPEND DEBUG_SNIPPETS
            "console"
            "debug"
            "complete-shell"
    )

    AddSnippets(${DEBUG_SNIPPETS})
endfunction()

function(AddSimSnippets)
    list(APPEND SIM_SNIPPETS
            "sim-periph"
    )

    AddSnippets(${SIM_SNIPPETS})
endfunction()

function(AddCompilerWarningsSnippets)
    list(APPEND COMPILER_WARNINGS_SNIPPETS
            "compiler-warnings"
    )

    AddSnippets(${COMPILER_WARNINGS_SNIPPETS})
endfunction()

function(AddAsanSnippets)
    list(APPEND ASAN_SNIPPETS
            "asan"
    )

    AddSnippets(${ASAN_SNIPPETS})
endfunction()

function(AddUbsanSnippets)
    list(APPEND UBSAN_SNIPPETS
            "ubsan"
    )

    AddSnippets(${UBSAN_SNIPPETS})
endfunction()
