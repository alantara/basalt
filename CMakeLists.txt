cmake_minimum_required(VERSION 3.23)
project(Basalt VERSION 1.0)

add_executable(basalt)
target_sources(basalt PRIVATE main.cpp)
target_link_libraries(basalt PRIVATE glfw vulkan)
