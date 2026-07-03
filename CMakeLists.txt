cmake_minimum_required(VERSION 3.23)

project(Basalt)
add_executable(basalt)
target_sources(basalt PRIVATE main.cpp)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
target_link_libraries(basalt PRIVATE glfw Vulkan::Vulkan)
