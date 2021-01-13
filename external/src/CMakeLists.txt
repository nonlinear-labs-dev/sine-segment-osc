find_package(PkgConfig REQUIRED)
set(LIBS ${LIBS} gtkmm-3.0 alsa libsoup-2.4)

file(GLOB_RECURSE ${PROJECT_NAME}_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*)
file(GLOB_RECURSE ${PROJECT_NAME}_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../*.h)
add_library(${PROJECT_NAME} ${${PROJECT_NAME}_FILES} ${${PROJECT_NAME}_HEADER_FILES})


find_package(PkgConfig REQUIRED)
set(LIBS ${LIBS} alsa)

foreach(lib ${LIBS})
  pkg_check_modules(${lib} REQUIRED ${lib})
  target_include_directories(${PROJECT_NAME} PUBLIC ${${lib}_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PUBLIC ${${lib}_LIBRARIES})
  link_directories(${${lib}_LIBRARY_DIRS})
endforeach(lib)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
