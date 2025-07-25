project(triangle)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)

add_executable (triangle
	hello_triangle.cpp)

target_compile_features(triangle PRIVATE cxx_std_20)

target_compile_definitions(triangle PUBLIC
	VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
	VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
	SHADER_DIR="${SHADER_DIR}/"
)

target_include_directories(triangle PUBLIC
	${CMAKE_SOURCE_DIR}/external/glfw/include/
)

target_link_libraries(triangle PRIVATE
	Vulkan::Vulkan
	glfw
)
