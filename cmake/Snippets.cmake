function(AddCommonSnippets)
    list(APPEND COMMON_SNIPPETS
            "bpl-networking"
            "cpp"
            "lfs"
    )

    foreach(snippet ${COMMON_SNIPPETS})
      if(NOT snippet IN_LIST SNIPPET)
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
      if(NOT snippet IN_LIST SNIPPET)
        list(APPEND SNIPPET ${snippet})
      endif()
    endforeach()

    set(SNIPPET ${SNIPPET} CACHE STRING "List of snippets" FORCE)
endfunction()