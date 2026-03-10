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

if (NOT WIN32)
    message(FATAL_ERROR "Local FMOD integration is configured only for Windows.")
endif()

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Local FMOD integration is configured only for x64 builds.")
endif()

set(FMOD_LIB_DIR "${CMAKE_SOURCE_DIR}/src/thirdparty/fmod_x64")
set(FMOD_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/thirdparty/fmod/include")

set(FMOD_REQUIRED_FILES
    "${FMOD_LIB_DIR}/fmod_vc.lib"
    "${FMOD_LIB_DIR}/fmodL_vc.lib"
    "${FMOD_LIB_DIR}/fmod.dll"
    "${FMOD_LIB_DIR}/fmodL.dll"
    "${FMOD_INCLUDE_DIR}/fmod.hpp"
    "${FMOD_INCLUDE_DIR}/fmod_errors.h"
)

foreach(FMOD_REQUIRED_FILE IN LISTS FMOD_REQUIRED_FILES)
    if (NOT EXISTS "${FMOD_REQUIRED_FILE}")
        message(FATAL_ERROR "Missing FMOD dependency file: ${FMOD_REQUIRED_FILE}")
    endif()
endforeach()

add_library(fmod::fmod SHARED IMPORTED GLOBAL)
set_target_properties(fmod::fmod PROPERTIES
    IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo;MinSizeRel"
    IMPORTED_IMPLIB_DEBUG "${FMOD_LIB_DIR}/fmodL_vc.lib"
    IMPORTED_LOCATION_DEBUG "${FMOD_LIB_DIR}/fmodL.dll"
    IMPORTED_IMPLIB_RELEASE "${FMOD_LIB_DIR}/fmod_vc.lib"
    IMPORTED_LOCATION_RELEASE "${FMOD_LIB_DIR}/fmod.dll"
    IMPORTED_IMPLIB_RELWITHDEBINFO "${FMOD_LIB_DIR}/fmod_vc.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO "${FMOD_LIB_DIR}/fmod.dll"
    IMPORTED_IMPLIB_MINSIZEREL "${FMOD_LIB_DIR}/fmod_vc.lib"
    IMPORTED_LOCATION_MINSIZEREL "${FMOD_LIB_DIR}/fmod.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${FMOD_INCLUDE_DIR}"
)

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

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE dbghelp psapi)
endif()

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

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FMOD_LIB_DIR}/fmod.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FMOD_LIB_DIR}/fmodL.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)
