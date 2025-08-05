# learn-vulkan

From [vulkan.org/learn](https://www.vulkan.org/learn)

- [Kronos Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest)

## Dependencies

- [LunarG Vulkan SDK](https://vulkan.lunarg.com/)
- [GLFW](https://github.com/glfw/glfw)

## Build Instructions

Clone the repo with

```bash
git clone --recurse-submodules git@github.com:joonsuuh/learn-vulkan.git
```

Generate build system

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

then build the project with

```bash
cmake --build build
```

## Chapter 1: Drawing a Triangle

Following [Drawing a triangle](https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/00_Base_code.html).

Notes:

- Swapchain recreation: validation layer error on windows...
