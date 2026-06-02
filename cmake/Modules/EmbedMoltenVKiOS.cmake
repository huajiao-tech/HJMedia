if(NOT DEFINED EMBED_APP_FRAMEWORKS_DIR OR EMBED_APP_FRAMEWORKS_DIR STREQUAL "")
    message(FATAL_ERROR "EMBED_APP_FRAMEWORKS_DIR is required")
endif()

if(NOT DEFINED EMBED_MOLTENVK_FRAMEWORK_SRC OR EMBED_MOLTENVK_FRAMEWORK_SRC STREQUAL "")
    message(FATAL_ERROR "EMBED_MOLTENVK_FRAMEWORK_SRC is required")
endif()

if(NOT DEFINED EMBED_MOLTENVK_BINARY_SRC OR EMBED_MOLTENVK_BINARY_SRC STREQUAL "")
    message(FATAL_ERROR "EMBED_MOLTENVK_BINARY_SRC is required")
endif()

set(_framework_dst "${EMBED_APP_FRAMEWORKS_DIR}/MoltenVK.framework")
set(_binary_dst "${EMBED_APP_FRAMEWORKS_DIR}/libvulkan.1.dylib")

execute_process(COMMAND "${CMAKE_COMMAND}" -E make_directory "${EMBED_APP_FRAMEWORKS_DIR}"
    RESULT_VARIABLE _mkdir_ret)
if(NOT _mkdir_ret EQUAL 0)
    message(FATAL_ERROR "Failed to create frameworks dir: ${EMBED_APP_FRAMEWORKS_DIR}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_directory "${EMBED_MOLTENVK_FRAMEWORK_SRC}" "${_framework_dst}"
    RESULT_VARIABLE _copy_fw_ret)
if(NOT _copy_fw_ret EQUAL 0)
    message(FATAL_ERROR "Failed to copy MoltenVK.framework")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${EMBED_MOLTENVK_BINARY_SRC}" "${_binary_dst}"
    RESULT_VARIABLE _copy_bin_ret)
if(NOT _copy_bin_ret EQUAL 0)
    message(FATAL_ERROR "Failed to copy libvulkan.1.dylib")
endif()

if(DEFINED ENV{EXPANDED_CODE_SIGN_IDENTITY} AND NOT "$ENV{EXPANDED_CODE_SIGN_IDENTITY}" STREQUAL "")
    execute_process(
        COMMAND /usr/bin/codesign --force --sign "$ENV{EXPANDED_CODE_SIGN_IDENTITY}" --preserve-metadata=identifier,entitlements,flags "${_framework_dst}"
        RESULT_VARIABLE _sign_fw_ret
        ERROR_VARIABLE _sign_fw_err)
    if(NOT _sign_fw_ret EQUAL 0)
        message(FATAL_ERROR "Failed to sign MoltenVK.framework: ${_sign_fw_err}")
    endif()

    execute_process(
        COMMAND /usr/bin/codesign --force --sign "$ENV{EXPANDED_CODE_SIGN_IDENTITY}" --preserve-metadata=identifier,entitlements,flags "${_binary_dst}"
        RESULT_VARIABLE _sign_bin_ret
        ERROR_VARIABLE _sign_bin_err)
    if(NOT _sign_bin_ret EQUAL 0)
        message(FATAL_ERROR "Failed to sign libvulkan.1.dylib: ${_sign_bin_err}")
    endif()
endif()
