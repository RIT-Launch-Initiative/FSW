function(AddCommonSnippets)
    list(APPEND COMMON_SNIPPETS
            "bpl-networking"
            "cpp"
            "lfs"
    )

    foreach(snippet ${COMMON_SNIPPETS})
        list(FIND SNIPPET ${snippet} snippet_index)
        if(snippet_index EQUAL -1)
            list(APPEND SNIPPET ${snippet})
        endif()
    endforeach()

    set(SNIPPET ${SNIPPET} CACHE STRING "List of snippets" FORCE)
endfunction()

function(AddDebugSnippets)
    list(APPEND DEBUG_SNIPPETS
            "console"
            "debug"
            "shell"
    )

    foreach(snippet ${DEBUG_SNIPPETS})
        list(FIND SNIPPET ${snippet} snippet_index)
        if(snippet_index EQUAL -1)
            list(APPEND SNIPPET ${snippet})
        endif()
    endforeach()

    set(SNIPPET ${SNIPPET} CACHE STRING "List of snippets" FORCE)
endfunction()
