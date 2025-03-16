cmake_minimum_required(VERSION 3.21)
project(azha LANGUAGES CXX C)

add_executable(azha src/main.cpp)
add_subdirectory(src/)
add_subdirectory(externals/glfw)
add_subdirectory(externals/glm)
add_subdirectory(externals/VulkanMemoryAllocator/)

target_include_directories(azha PRIVATE include/ externals/glfw externals/glm)
target_compile_features(azha PRIVATE cxx_std_17 c_std_99)

target_precompile_headers(azha PUBLIC include/stdafx.hpp)

target_link_libraries(azha PRIVATE GPUOpen::VulkanMemoryAllocator glfw glm vulkan)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_SOURCE_DIR}/Assets/"
        "${CMAKE_BINARY_DIR}/Assets"
    COMMENT "Copying Assets folder"
)

add_dependencies(azha copy_assets)

install(TARGETS azha)
