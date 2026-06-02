if(NOT DEFINED src OR NOT DEFINED dst)
    message(FATAL_ERROR "copy script missing variables: src=${src}, dst=${dst}")
endif()

if(NOT EXISTS "${dst}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${src}" "${dst}"
        RESULT_VARIABLE _copy_result
    )
    if(NOT _copy_result EQUAL 0)
        message(FATAL_ERROR "Failed to copy ${src} to ${dst}, result ${_copy_result}")
    endif()
else()
    message(STATUS "Skip copy, destination already exists: ${dst}")
endif()
