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

file(GLOB_RECURSE ATH_ALL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
list(FILTER ATH_ALL_SOURCES EXCLUDE REGEX ".*/src/thirdparty/imgui_backends/.*\\.cpp$")

file(GLOB IMGUI_BACKEND_SOURCES
    "${CMAKE_SOURCE_DIR}/src/thirdparty/imgui_backends/*.cpp"
)

set(ATH_EDITOR_SOURCES ${ATH_ALL_SOURCES})
list(FILTER ATH_EDITOR_SOURCES EXCLUDE REGEX ".*/src/runtime_main\\.cpp$")
list(FILTER ATH_EDITOR_SOURCES EXCLUDE REGEX ".*/src/app/RuntimeApplication\\.cpp$")
list(FILTER ATH_EDITOR_SOURCES EXCLUDE REGEX ".*/src/scene/RuntimeSceneManager\\.cpp$")
list(FILTER ATH_EDITOR_SOURCES EXCLUDE REGEX ".*/src/scene/scenes/RuntimeGameScene\\.cpp$")

set(ATH_RUNTIME_SOURCES ${ATH_ALL_SOURCES})
list(FILTER ATH_RUNTIME_SOURCES EXCLUDE REGEX ".*/src/editor/.*\\.cpp$")
list(FILTER ATH_RUNTIME_SOURCES EXCLUDE REGEX ".*/src/editor_main\\.cpp$")
list(FILTER ATH_RUNTIME_SOURCES EXCLUDE REGEX ".*/src/app/Application\\.cpp$")
list(FILTER ATH_RUNTIME_SOURCES EXCLUDE REGEX ".*/src/scene/SceneManager\\.cpp$")
list(FILTER ATH_RUNTIME_SOURCES EXCLUDE REGEX ".*/src/scene/SceneRuntime\\.cpp$")
list(FILTER ATH_RUNTIME_SOURCES EXCLUDE REGEX ".*/src/scene/Scene\\.cpp$")
list(FILTER ATH_RUNTIME_SOURCES EXCLUDE REGEX ".*/src/scene/EditorSceneIO\\.cpp$")
list(FILTER ATH_RUNTIME_SOURCES EXCLUDE REGEX ".*/src/scene/scenes/CoreScene\\.cpp$")
list(FILTER ATH_RUNTIME_SOURCES EXCLUDE REGEX ".*/src/debugviz/.*\\.cpp$")
list(FILTER ATH_RUNTIME_SOURCES EXCLUDE REGEX ".*/src/profiler/Profiler\\.cpp$")
list(FILTER ATH_RUNTIME_SOURCES EXCLUDE REGEX ".*/src/tools/SceneValidation\\.cpp$")
list(FILTER ATH_RUNTIME_SOURCES EXCLUDE REGEX ".*/src/thirdparty/ImGuizmo\\.cpp$")

add_executable(athengine_editor ${ATH_EDITOR_SOURCES})
add_executable(athengine_runtime ${ATH_RUNTIME_SOURCES})

function(configure_ath_target target_name)
    target_link_libraries(${target_name}
        PRIVATE
        glfw
        GLEW::GLEW
        glm::glm
        assimp::assimp
        fmod::fmod
    )

    if (WIN32)
        target_link_libraries(${target_name} PRIVATE dbghelp psapi)
    endif()

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res/shaders
        $<TARGET_FILE_DIR:${target_name}>/shader
    )

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FMOD_LIB_DIR}/fmod.dll"
            "$<TARGET_FILE_DIR:${target_name}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FMOD_LIB_DIR}/fmodL.dll"
            "$<TARGET_FILE_DIR:${target_name}>"
    )

    target_compile_definitions(${target_name} PRIVATE
        ASSET_PATH="${CMAKE_SOURCE_DIR}/res"
    )
endfunction()

configure_ath_target(athengine_editor)
configure_ath_target(athengine_runtime)

target_link_libraries(athengine_editor PRIVATE imgui::imgui)
target_sources(athengine_editor PRIVATE ${IMGUI_BACKEND_SOURCES})
target_include_directories(athengine_editor PRIVATE
    "${CMAKE_SOURCE_DIR}/src/thirdparty/imgui_backends"
)
target_compile_definitions(athengine_editor PRIVATE ATH_EDITOR_BUILD=1)
target_compile_definitions(athengine_runtime PRIVATE ATH_RUNTIME_BUILD=1)
