cmake_minimum_required(VERSION 3.24)
project(nice7z C)

set(CMAKE_C_STANDARD 11)

include(7zip.toolchain.cmake)
include_directories(7zip/C)
