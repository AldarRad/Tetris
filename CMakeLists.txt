cmake_minimum_required(VERSION 3.14)
project(titris VERSION 0.1.0 LANGUAGES C CXX)

add_executable(titris main.cpp)

add_library(g Game.cpp)

target_link_libraries(titris g)

set(SFML_DIR "D:\\SFML\\SFML-2.6.2-windows-gcc-13.1.0-mingw-64-bit\\SFML-2.6.2")

find_package(SFML 2.5 COMPONENTS system window graphics network audio REQUIRED)


include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


target_link_libraries(titris sfml-system sfml-window sfml-graphics sfml-audio) 