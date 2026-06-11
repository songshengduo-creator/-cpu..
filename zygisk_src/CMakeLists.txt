cmake_minimum_required(VERSION 3.18)
project(cpuinfo_spoofer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR})

add_library(cpuinfo_spoofer SHARED main.cpp)

target_link_libraries(cpuinfo_spoofer log)

target_link_options(cpuinfo_spoofer PRIVATE
    -Wl,--strip-all
    -Wl,--exclude-libs,ALL
)

set_target_properties(cpuinfo_spoofer PROPERTIES
    CXX_VISIBILITY_PRESET hidden
)
