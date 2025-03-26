cmake_minimum_required(VERSION 3.20)

project(oak CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

add_compile_definitions(HOWLER_PREFIX="oak")

set(HOWLER_FMT_EXTERNAL TRUE)
add_subdirectory(thirdparty/fmt)
add_subdirectory(thirdparty/howler)

include_directories(include
	thirdparty
	thirdparty/glm)

add_library(oak STATIC
	source/device.cpp
	source/device-resources.cpp
	source/image.cpp
	source/globals.cpp
	source/pfn.cpp
	source/render-pass.cpp
	source/sbt.cpp
	source/spirv.cpp
	source/util.cpp
	source/window.cpp)

target_link_libraries(oak PRIVATE howler glfw Vulkan::Vulkan)

add_executable(hello_triangle
	examples/hello_triangle.cpp)

add_executable(spinning_cube
	examples/spinning_cube.cpp)

target_link_libraries(hello_triangle PRIVATE oak)
target_link_libraries(spinning_cube  PRIVATE oak)
