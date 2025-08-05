#learn-vulkan

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

## Chapter 0: Intro

Just a simple test to see if the project builds properly with GLFW.
