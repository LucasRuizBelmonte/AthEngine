cmake_minimum_required(VERSION 3.24)
project(athengine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 REQUIRED)
find_package(glew REQUIRED)
find_package(glm REQUIRED)
find_package(imgui REQUIRED)
find_package(assimp REQUIRED)
find_package(fmod REQUIRED)

file(GLOB_RECURSE ATH_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
list(FILTER ATH_SOURCES EXCLUDE REGEX ".*/src/thirdparty/imgui_backends/.*\\.cpp$")
list(FILTER ATH_SOURCES EXCLUDE REGEX ".*/src/thirdparty/imgui_backendsWrong/.*\\.cpp$")

add_executable(${PROJECT_NAME} ${ATH_SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    glfw
    GLEW::GLEW
    glm::glm
    imgui::imgui
    assimp::assimp
    fmod::fmod
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shader
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ASSET_PATH="${CMAKE_SOURCE_DIR}/res"
)

file(GLOB IMGUI_BACKEND_SOURCES
    "${CMAKE_SOURCE_DIR}/src/thirdparty/imgui_backends/*.cpp"
)

target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_BACKEND_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/src/thirdparty/imgui_backends"
)

if (WIN32 AND DEFINED ENV{FMOD_SDK_DIR})
    set(FMOD_DLL_DIR "$ENV{FMOD_SDK_DIR}/api/core/lib/x64")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FMOD_DLL_DIR}/fmod.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )

    if (EXISTS "${FMOD_DLL_DIR}/fmodL.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FMOD_DLL_DIR}/fmodL.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
    endif()
endif()
