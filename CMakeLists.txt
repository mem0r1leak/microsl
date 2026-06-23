cmake_minimum_required(VERSION 4.2)
project(microsl)

set(CMAKE_CXX_STANDARD 20)

add_library(microsl INTERFACE
        include/microsl/types.hpp
        include/microsl/span.hpp
        include/microsl/endian.hpp
        include/microsl/streaming.hpp
        include/microsl/allocator.hpp)

target_include_directories(microsl INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
