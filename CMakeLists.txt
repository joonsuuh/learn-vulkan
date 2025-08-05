cmake_minimum_required(VERSION 3.30)

project(learn_vulkan)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders)

find_package(Vulkan REQUIRED)

add_subdirectory(external/glfw)

add_executable(learn_vulkan
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_compile_features(learn_vulkan PRIVATE cxx_std_20)

target_compile_definitions(learn_vulkan PRIVATE
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    VK_ENABLE_BETA_EXTENSIONS=1
    SHADER_DIR="${SHADER_DIR}/"
)

target_include_directories(learn_vulkan PRIVATE
    $ENV{VULKAN_SDK}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include/
)

target_link_libraries(learn_vulkan PRIVATE
    Vulkan::Vulkan
    glfw
)

