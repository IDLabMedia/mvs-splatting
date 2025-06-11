cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(MVSSplatting)

# Add OpenGL, GLFW
find_package(OpenGL REQUIRED)
add_subdirectory(include/glfw)

# Include headers
include_directories(include/glm include/glad include/imgui include/stb_image include/cxxopts)

file(GLOB SRC_FILES src/*.cpp src/*.h include/glad/gl.c)
file(GLOB IMGUI_SOURCES include/imgui/*.cpp include/imgui/*.h)
file(GLOB SHADER_FILES src/shaders/*.*)
source_group("imgui" FILES ${IMGUI_SOURCES} )
source_group("shaders" FILES ${SHADER_FILES})

add_executable(${PROJECT_NAME} ${SRC_FILES} ${IMGUI_SOURCES} ${SHADER_FILES})
target_link_libraries(${PROJECT_NAME} OpenGL::GL glfw )
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

if (MSVC)
	# set startup project
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif (MSVC)

target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKELISTS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")