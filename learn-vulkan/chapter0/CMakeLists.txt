project(intro)

add_executable (intro
	main.cpp)

target_compile_features(intro PRIVATE cxx_std_20)

target_include_directories(intro PUBLIC
	${CMAKE_SOURCE_DIR}/external/glfw/include/
)

target_link_libraries(intro PRIVATE
	Vulkan::Vulkan
	glfw
)
