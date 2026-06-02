if(NOT FACEU_IS_RELEASE)
    if(FACEU_ACTIVE_CONFIG)
        message(STATUS "Skipping Faceu iOS export for config '${FACEU_ACTIVE_CONFIG}'")
    else()
        message(STATUS "Skipping Faceu iOS export (non-Release build)")
    endif()
    return()
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" -E make_directory "${FACEU_DEMO_CORE_DIR}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E remove_directory "${FACEU_DEMO_CORE_DIR}/HJRender.framework")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_directory "${FACEU_FRAMEWORK_SRC}" "${FACEU_DEMO_CORE_DIR}/HJRender.framework")

execute_process(COMMAND "${CMAKE_COMMAND}" -E remove_directory "${FACEU_DEMO_SOURCE_DIR}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_directory "${FACEU_SOURCE_DIR}" "${FACEU_DEMO_SOURCE_DIR}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E rm -f "${FACEU_DEMO_SOURCE_DIR}/HJExport.h")

execute_process(COMMAND "${CMAKE_COMMAND}" -E remove_directory "${FACEU_DEMO_RESOURCE_DIR}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_directory "${FACEU_APP_RESOURCE_DIR}" "${FACEU_DEMO_RESOURCE_DIR}")
