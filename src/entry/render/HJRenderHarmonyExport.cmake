set(HJ_HARMONY_EXPORT_ROOT ${CMAKE_SOURCE_DIR}/demo/faceu/harmony/Export/core)
set(HJ_HARMONY_EXPORT_LIB_DIR ${HJ_HARMONY_EXPORT_ROOT}/lib)
set(HJ_HARMONY_EXPORT_INCLUDE_DIR ${HJ_HARMONY_EXPORT_ROOT}/include)
set(HJ_HARMONY_EXPORT_RESFILE_DIR ${CMAKE_SOURCE_DIR}/demo/faceu/harmony/Export/demo/entry/src/main/resources/resfile)
set(HJ_HARMONY_EXPORT_RESFILE_RESOURCE_DIR ${HJ_HARMONY_EXPORT_RESFILE_DIR}/resource)
set(HJ_HARMONY_EXPORT_RESOURCE_SRC ${CMAKE_SOURCE_DIR}/examples/resource)

set(HJ_HARMONY_EXPORT_HEADERS
    ${CMAKE_SOURCE_DIR}/src/entry/render/HJRenderContextExport.h
    ${CMAKE_SOURCE_DIR}/src/entry/render/HJRenderFaceuExport.h
    ${CMAKE_SOURCE_DIR}/src/entry/HJCommonInterface.h
    ${CMAKE_SOURCE_DIR}/src/utils/base/HJExport.h
)

add_custom_command(TARGET ${PROJ_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${HJ_HARMONY_EXPORT_LIB_DIR}
        ${HJ_HARMONY_EXPORT_INCLUDE_DIR}
        ${HJ_HARMONY_EXPORT_RESFILE_RESOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJ_NAME}>
        ${HJ_HARMONY_EXPORT_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${HJ_HARMONY_EXPORT_HEADERS}
        ${HJ_HARMONY_EXPORT_INCLUDE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${HJ_HARMONY_EXPORT_RESOURCE_SRC}
        ${HJ_HARMONY_EXPORT_RESFILE_RESOURCE_DIR}
    COMMENT "Export ${PROJ_NAME} Harmony artifacts for the faceu demo"
)
