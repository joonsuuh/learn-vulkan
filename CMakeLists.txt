cmake_minimum_required (VERSION 3.30)

project ("learn-vulkan")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)

add_subdirectory(external/glfw)
add_subdirectory("learn-vulkan")
