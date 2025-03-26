cmake_minimum_required(VERSION 3.20)

project(oak CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_package(assimp REQUIRED)

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
	source/queue.cpp
	source/sbt.cpp
	source/spirv.cpp
	source/util.cpp
	source/window.cpp)

target_link_libraries(oak PRIVATE howler glfw Vulkan::Vulkan)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	add_executable(hello-triangle
		examples/hello-triangle.cpp)

	add_executable(spinning-cube
		examples/spinning-cube.cpp)

	add_executable(mesh-viewer
		examples/mesh-viewer.cpp)

	add_executable(model-viewer
		examples/model-viewer.cpp)

	target_link_libraries(hello-triangle PRIVATE oak)
	target_link_libraries(spinning-cube  PRIVATE oak)
	target_link_libraries(mesh-viewer    PRIVATE oak assimp)
	target_link_libraries(model-viewer   PRIVATE oak assimp)
endif()
