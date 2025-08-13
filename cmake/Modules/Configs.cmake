#**********************************************************************************#
add_definitions(-DHJ_HAVE_YYJSON)
add_definitions(-DHJ_HAVE_ARENDER_MINI)

#**********************************************************************************#
# Define work dir and sub dirs
#**********************************************************************************#
set(HJ_DEPS_DIR ${CMAKE_SOURCE_DIR}/externals/${PLATFORM_DIR})
# message(STATUS "HJ_DEPS_DIR:${HJ_DEPS_DIR}")
