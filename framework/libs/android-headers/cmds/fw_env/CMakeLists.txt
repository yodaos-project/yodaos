aux_source_directory(. DIR_SRCS)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(fw_printenv ${DIR_SRCS})

install(TARGETS fw_printenv DESTINATION tool_env/)

