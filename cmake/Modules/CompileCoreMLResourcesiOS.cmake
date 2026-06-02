if(NOT DEFINED COREML_SOURCE_DIR OR COREML_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "COREML_SOURCE_DIR is required")
endif()

if(NOT DEFINED COREML_BUNDLE_DIR OR COREML_BUNDLE_DIR STREQUAL "")
    message(FATAL_ERROR "COREML_BUNDLE_DIR is required")
endif()

if(NOT EXISTS "${COREML_SOURCE_DIR}")
    message(STATUS "CoreML source dir not found, skip: ${COREML_SOURCE_DIR}")
    return()
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${COREML_BUNDLE_DIR}"
    RESULT_VARIABLE _bundle_rm_ret
)
if(NOT _bundle_rm_ret EQUAL 0)
    message(FATAL_ERROR "Failed to clear CoreML bundle dir: ${COREML_BUNDLE_DIR}")
endif()

file(MAKE_DIRECTORY "${COREML_BUNDLE_DIR}")

file(GLOB_RECURSE _coreml_metadata_files LIST_DIRECTORIES FALSE "${COREML_SOURCE_DIR}/metadata.json")
set(_coreml_compiled_model_dirs)
foreach(_compiled_meta ${_coreml_metadata_files})
    if(NOT _compiled_meta MATCHES "\\.mlmodelc/metadata\\.json$")
        continue()
    endif()
    get_filename_component(_compiled_dir "${_compiled_meta}" DIRECTORY)
    if(_compiled_dir MATCHES "\\.mlpackage/")
        continue()
    endif()
    list(APPEND _coreml_compiled_model_dirs "${_compiled_dir}")
endforeach()
list(REMOVE_DUPLICATES _coreml_compiled_model_dirs)

foreach(_compiled_dir ${_coreml_compiled_model_dirs})
    file(RELATIVE_PATH _rel_dir "${COREML_SOURCE_DIR}" "${_compiled_dir}")
    get_filename_component(_dst_parent "${COREML_BUNDLE_DIR}/${_rel_dir}" DIRECTORY)
    get_filename_component(_name "${_compiled_dir}" NAME)
    set(_dst "${_dst_parent}/${_name}")
    file(MAKE_DIRECTORY "${_dst_parent}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${_dst}"
        RESULT_VARIABLE _rm_ret
    )
    if(NOT _rm_ret EQUAL 0)
        message(FATAL_ERROR "Failed to remove existing CoreML dir: ${_dst}")
    endif()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${_compiled_dir}" "${_dst}"
        RESULT_VARIABLE _copy_ret
    )
    if(NOT _copy_ret EQUAL 0)
        message(FATAL_ERROR "Failed to copy precompiled CoreML dir: ${_compiled_dir}")
    endif()
    message(STATUS "Copied precompiled CoreML model: ${_name}")
endforeach()

file(GLOB_RECURSE _coreml_manifest_files LIST_DIRECTORIES FALSE "${COREML_SOURCE_DIR}/Manifest.json")
file(GLOB_RECURSE _coreml_models LIST_DIRECTORIES FALSE "${COREML_SOURCE_DIR}/*.mlmodel")
set(_coreml_packages)
foreach(_manifest ${_coreml_manifest_files})
    if(NOT _manifest MATCHES "\\.mlpackage/Manifest\\.json$")
        continue()
    endif()
    get_filename_component(_package_dir "${_manifest}" DIRECTORY)
    if(IS_DIRECTORY "${_package_dir}")
        list(APPEND _coreml_packages "${_package_dir}")
    endif()
endforeach()
list(REMOVE_DUPLICATES _coreml_packages)

set(_coreml_source_models)
foreach(_model ${_coreml_models})
    if(_model MATCHES "\\.mlpackage/" OR _model MATCHES "\\.mlmodelc/")
        continue()
    endif()
    list(APPEND _coreml_source_models "${_model}")
endforeach()

set(_coreml_compile_inputs ${_coreml_packages} ${_coreml_source_models})

foreach(_src ${_coreml_compile_inputs})
    file(RELATIVE_PATH _rel_path "${COREML_SOURCE_DIR}" "${_src}")
    get_filename_component(_rel_parent "${_rel_path}" DIRECTORY)
    get_filename_component(_name_we "${_src}" NAME_WE)
    set(_out_parent "${COREML_BUNDLE_DIR}")
    if(NOT _rel_parent STREQUAL "")
        set(_out_parent "${COREML_BUNDLE_DIR}/${_rel_parent}")
    endif()
    file(MAKE_DIRECTORY "${_out_parent}")
    set(_out_dir "${_out_parent}/${_name_we}.mlmodelc")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${_out_dir}"
        RESULT_VARIABLE _rm_ret
    )
    if(NOT _rm_ret EQUAL 0)
        message(FATAL_ERROR "Failed to remove existing compiled CoreML dir: ${_out_dir}")
    endif()
    execute_process(
        COMMAND xcrun coremlcompiler compile "${_src}" "${_out_parent}"
        RESULT_VARIABLE _compile_ret
    )
    if(NOT _compile_ret EQUAL 0)
        message(FATAL_ERROR "Failed to compile CoreML model: ${_src}")
    endif()
    if(NOT EXISTS "${_out_dir}")
        message(FATAL_ERROR "Compiled CoreML output missing: ${_out_dir}")
    endif()
    message(STATUS "Compiled CoreML model: ${_src} -> ${_out_dir}")
endforeach()
