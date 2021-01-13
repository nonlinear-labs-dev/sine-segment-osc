file(GLOB_RECURSE ${PROJECT_NAME}_TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*)

add_executable(${PROJECT_NAME}-tests  ${${PROJECT_NAME}_TEST_FILES})
target_include_directories(${PROJECT_NAME}-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME}-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(${PROJECT_NAME}-tests PRIVATE ${PROJECT_NAME})

enable_testing()
