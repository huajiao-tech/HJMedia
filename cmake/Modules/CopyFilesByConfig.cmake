if(NOT DEFINED cfg OR NOT DEFINED dst)
    message(FATAL_ERROR "copy script missing variables: cfg=${cfg}, dst=${dst}")
endif()

if(cfg STREQUAL "Debug")
    set(_files "${debug_files}")
else()
    set(_files "${release_files}")
endif()

#message(STATUS "CopyFilesByConfig: cfg=${cfg}")
#message(STATUS "CopyFilesByConfig: dst=${dst}")
#message(STATUS "CopyFilesByConfig: files=${_files}")

foreach(_file IN LISTS _files)
    if(EXISTS "${_file}")
        #message(STATUS "CopyFilesByConfig: copying ${_file} -> ${dst}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_file}" "${dst}"
            RESULT_VARIABLE _copy_result
        )
        if(NOT _copy_result EQUAL 0)
            message(FATAL_ERROR "Failed to copy ${_file} to ${dst}, result ${_copy_result}")
        endif()
    else()
        message(WARNING "Skip missing file: ${_file}")
    endif()
endforeach()
