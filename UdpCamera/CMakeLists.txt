cmake_minimum_required(VERSION 2.6)
project(udpcamera)

set(CMAKE_CXX_FLAGS "-std=c++11")

add_executable(udpcamera main.cpp)

target_link_libraries(udpcamera avformat avcodec swscale avdevice avutil)

#install(TARGETS udpcamera RUNTIME DESTINATION bin)
