project(triangle)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)

add_executable (triangle
	hello_triangle.cpp)

target_compile_features(triangle PRIVATE cxx_std_20)

target_compile_definitions(triangle PRIVATE
	VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
	VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    VK_ENABLE_BETA_EXTENSIONS=1
	SHADER_DIR="${SHADER_DIR}/"
)

target_include_directories(triangle
    PRIVATE
    $ENV{VULKAN_SDK}/include
	${CMAKE_SOURCE_DIR}/external/glfw/include/
)

target_link_libraries(triangle PRIVATE
    Vulkan::Headers
	Vulkan::Vulkan
	glfw
)
