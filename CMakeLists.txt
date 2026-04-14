cmake_minimum_required(VERSION 3.16)
project(GesturesTool)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBINPUT REQUIRED libinput)
pkg_check_modules(LIBUDEV REQUIRED libudev)

add_executable(GesturesTool main.cpp)

target_include_directories(GesturesTool PRIVATE
        ${LIBINPUT_INCLUDE_DIRS}
        ${LIBUDEV_INCLUDE_DIRS}
)

target_link_libraries(GesturesTool PRIVATE
        ${LIBINPUT_LIBRARIES}
        ${LIBUDEV_LIBRARIES}
)