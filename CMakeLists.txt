cmake_minimum_required(VERSION 3.7)

project(HelloVulkan)

add_definitions(-DUNICODE)
set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/Source")

file(GLOB_RECURSE SRC SRC_DIR
    "${SRC_DIR}/*.cpp"
    "${SRC_DIR}/*.h"
    "${SRC_DIR}/*.hpp")

add_executable(${PROJECT_NAME} ${SRC})

# Copy shaders
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Shaders/" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Resources/Shaders/")

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")
set(DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies")

# Vulkan
find_package(Vulkan REQUIRED FATAL_ERROR)
target_compile_definitions(${PROJECT_NAME} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

# GLFW
set(GLFW_DIR "${DEPS_DIR}/GLFW")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory("${GLFW_DIR}")
target_link_libraries(${PROJECT_NAME} "glfw" "${GLFW_LIBRARIES}")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLFW_DIR}/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE "GLFW_INCLUDE_NONE")

# GLM
set(GLM_DIR "${DEPS_DIR}/GLM")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLM_DIR}")
