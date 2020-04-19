# Minimum CMake version
cmake_minimum_required(VERSION 3.7)

# Generated project name
project(HelloVulkan)

# C++ standard (C++17)
set(CMAKE_CXX_STANDARD 17)

# Add filters/folders for the generated projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Source files
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source")
set(DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies")

# Executable definition and properties
file(GLOB_RECURSE SRC
    "${SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE INC
    "${SOURCE_DIR}/*.h"
)

add_executable(${PROJECT_NAME} ${SRC} ${INC})

# Link the debug and release libraries to the project
target_link_libraries(${PROJECT_NAME} ${VULKAN_LIB_LIST})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

# Vulkan
find_package(Vulkan REQUIRED FATAL_ERROR)
target_compile_definitions(${PROJECT_NAME} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

# GLFW
set(GLFW_DIR "${DEPS_DIR}/glfw")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory("${GLFW_DIR}")
target_link_libraries(${PROJECT_NAME} "glfw" "${GLFW_LIBRARIES}")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLFW_DIR}/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE "GLFW_INCLUDE_NONE")

# GLM
set(GLM_DIR "${DEPS_DIR}/glm")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLM_DIR}")

# Debugging
message(STATUS "This is the BINARY dir " ${PROJECT_BINARY_DIR})
message(STATUS "This is the SOURCE dir " ${PROJECT_SOURCE_DIR})