project(external)

file(GLOB CORE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_subdirectory(src)
add_subdirectory(tests)