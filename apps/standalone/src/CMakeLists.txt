file(GLOB_RECURSE ${PROJECT_NAME}_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*)
add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE core external)