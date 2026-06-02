
set(COPY_UTILS_DIR ${CMAKE_CURRENT_LIST_DIR})

# Function to copy a directory to the target's output directory after build
function(copy_directory_post_build targetName sourceDir destDir)
    add_custom_command(TARGET ${targetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -Dsrc=${sourceDir}
            -Ddst=$<TARGET_FILE_DIR:${targetName}>/${destDir}
            -P ${COPY_UTILS_DIR}/CopyDirectoryIfMissing.cmake
        COMMENT "Copying directory ${sourceDir} to output directory ${destDir} if missing"
    )
endfunction()

# Function to copy a file to the target's output directory after build
function(copy_file_post_build targetName sourceFile destFile)
    add_custom_command(TARGET ${targetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${sourceFile}
        $<TARGET_FILE_DIR:${targetName}>/${destFile}
        COMMENT "Copying file ${sourceFile} to output directory"
    )
endfunction()

# Function to copy a list of files to the target's output directory after build
function(copy_files_post_build targetName debugFiles releaseFiles)
    set(_debug_files_arg "")
    set(_release_files_arg "")
    if(debugFiles)
        string(JOIN ";" _debug_files_arg ${debugFiles})
    endif()
    if(releaseFiles)
        string(JOIN ";" _release_files_arg ${releaseFiles})
    endif()
    add_custom_command(TARGET ${targetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -Dcfg=$<CONFIG>
            -Ddst=$<TARGET_FILE_DIR:${targetName}>
            "-Ddebug_files=${_debug_files_arg}"
            "-Drelease_files=${_release_files_arg}"
            -P ${COPY_UTILS_DIR}/CopyFilesByConfig.cmake
        COMMENT "Copying files to output directory for $<CONFIG>"
    )
endfunction()

# Function to copy a list of files to a specific directory after build
function(copy_files_to_dir_post_build targetName debugFiles releaseFiles destDir)
    set(_debug_files_arg "")
    set(_release_files_arg "")
    if(debugFiles)
        string(JOIN ";" _debug_files_arg ${debugFiles})
    endif()
    if(releaseFiles)
        string(JOIN ";" _release_files_arg ${releaseFiles})
    endif()
    add_custom_command(TARGET ${targetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -Dcfg=$<CONFIG>
            -Ddst="${destDir}"
            "-Ddebug_files=${_debug_files_arg}"
            "-Drelease_files=${_release_files_arg}"
            -P ${COPY_UTILS_DIR}/CopyFilesByConfig.cmake
        COMMENT "Copying files to ${destDir} for $<CONFIG>"
    )
endfunction()
