cmake_minimum_required(VERSION 3.10)

project(demo C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include("${CMAKE_CURRENT_SOURCE_DIR}/local.cmake" OPTIONAL)
find_package(SDL3 3.4 REQUIRED CONFIG)
message(STATUS "SDL3 config: ${SDL3_DIR}")

add_executable(${PROJECT_NAME} 
    src/main.c
    src/shaders.c
    src/img.c

    src/000_clear_screen.c
    src/001_fixed_triangle.c
    src/002_basic_triangle_vertex_buffer.c
    src/003_fixed_quad.c
    src/004_basic_index_buffer.c
    src/005_basic_uniform.c
    src/006_basic_texture.c
    src/basic_3d_pyramid.c
    src/pong.c
    src/basic_compute.c
    src/hex_fractal_stateless.c
    
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying SDL3.dll"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        ${CMAKE_SOURCE_DIR}/shaders/compiled 
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/compiled
    COMMENT "Copying compiled shaders"
)


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        ${CMAKE_SOURCE_DIR}/assets 
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying assets"
)