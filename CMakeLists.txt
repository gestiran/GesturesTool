cmake_minimum_required(VERSION 3.16)
project(GesturesTool)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVDEV REQUIRED libevdev)

add_executable(GesturesTool main.cpp
        main.h)

target_include_directories(GesturesTool PRIVATE ${LIBEVDEV_INCLUDE_DIRS})
target_link_libraries(GesturesTool PRIVATE ${LIBEVDEV_LIBRARIES})