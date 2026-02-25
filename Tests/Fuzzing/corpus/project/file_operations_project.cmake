add_executable(fileapp main.c)

file(WRITE "${CMAKE_BINARY_DIR}/generated.h" "#pragma once\n#define GENERATED 1\n")
file(WRITE "${CMAKE_BINARY_DIR}/config.txt" "key=value\n")

configure_file("${CMAKE_SOURCE_DIR}/include/config.h" "${CMAKE_BINARY_DIR}/config.h" @ONLY)

file(COPY "${CMAKE_SOURCE_DIR}/include/" DESTINATION "${CMAKE_BINARY_DIR}/copied_include"
  FILES_MATCHING PATTERN "*.h"
)

file(GLOB sources "${CMAKE_SOURCE_DIR}/*.c")
file(GLOB_RECURSE all_sources "${CMAKE_SOURCE_DIR}/*.c" "${CMAKE_SOURCE_DIR}/*.cpp")

file(RELATIVE_PATH relpath "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/output/sub1/sub2")
file(TOUCH "${CMAKE_BINARY_DIR}/output/stamp.txt")

target_include_directories(fileapp PRIVATE "${CMAKE_BINARY_DIR}")

install(FILES "${CMAKE_BINARY_DIR}/generated.h" DESTINATION include)
install(DIRECTORY "${CMAKE_BINARY_DIR}/copied_include/" DESTINATION include)
