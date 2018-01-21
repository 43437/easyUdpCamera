cmake_minimum_required(VERSION 2.6)
project(udpcameraclient)

add_executable(udpcameraclient main.cpp)

target_link_libraries(udpcameraclient avformat avcodec avutil swscale SDL2)
#install(TARGETS udpcameraclient RUNTIME DESTINATION bin)
