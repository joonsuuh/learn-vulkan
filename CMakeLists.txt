cmake_minimum_required(VERSION 3.30)

project(learn_vulkan)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)

add_subdirectory(external/glfw)

add_executable(learn_vulkan
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_compile_features(learn_vulkan PRIVATE cxx_std_20)

target_include_directories(learn_vulkan PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include/
)

target_link_libraries(learn_vulkan PRIVATE
    Vulkan::Vulkan
    glfw
)
