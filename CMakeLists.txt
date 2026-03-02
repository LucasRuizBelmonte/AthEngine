cmake_minimum_required(VERSION 3.24)
project(athengine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

file(GLOB_RECURSE ATH_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(${PROJECT_NAME} ${ATH_SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    glfw
    GLEW::GLEW
    glm::glm
)

# Copy shaders from src/shader
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res/shader
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shader
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ASSET_PATH="${CMAKE_SOURCE_DIR}/res"
)