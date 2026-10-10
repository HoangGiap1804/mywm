cmake_minimum_required(VERSION 3.10)
project(mywm VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wall -Wextra)
add_compile_definitions(WLR_USE_UNSTABLE)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WLROOTS REQUIRED wlroots-0.20)
pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

add_executable(mywm main.cpp)

target_include_directories(mywm PRIVATE 
    ${WLROOTS_INCLUDE_DIRS}
    ${WAYLAND_SERVER_INCLUDE_DIRS}
    ${XKBCOMMON_INCLUDE_DIRS}
)
target_link_libraries(mywm PRIVATE 
    ${WLROOTS_LIBRARIES}
    ${WAYLAND_SERVER_LIBRARIES}
    ${XKBCOMMON_LIBRARIES}
)
