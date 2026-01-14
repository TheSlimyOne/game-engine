cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0091 NEW)         # Make MSVC_RUNTIME_LIBRARY effective
cmake_policy(SET CMP0077 NEW)         # option() honors normal variables

project(Game)

# Use dynamic MSVC runtime: /MD (Release) and /MDd (Debug)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


include(FetchContent)

# =========== Dependencies ===========

# Set options BEFORE making them available
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_MSVC_STATIC_RUNTIME OFF CACHE BOOL "Use dynamic MSVC runtime for Assimp") # Force Assimp to use the dynamic runtime (/MD or /MDd)

set(BGFX_INSTALL ON CACHE BOOL "Install bgfx")
set(BGFX_BUILD_TOOLS ON CACHE BOOL "Build bgfx tools")

# Jolt: ensure dynamic runtime (not /MT)
set(MSVC_STATIC_RUNTIME OFF CACHE BOOL "Use dynamic runtime for Jolt")

# bgfx
FetchContent_Declare(
    bgfx
    GIT_REPOSITORY https://github.com/widberg/bgfx.cmake.git
    GIT_TAG master
)

# glfw
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

# assimp
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v6.0.2
)

# imgui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.1
)

# jolt physics
FetchContent_Declare(
    joltphysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG v5.3.0
    SOURCE_SUBDIR Build
)

FetchContent_MakeAvailable(bgfx glfw assimp imgui joltphysics)

# Add Jolt explicitly so our cache options apply and we can tweak its target
FetchContent_GetProperties(joltphysics)
if(NOT joltphysics_POPULATED)
    FetchContent_Populate(joltphysics)
    # Ensure dynamic runtime on the Jolt target as an extra guarantee
    add_subdirectory(${joltphysics_SOURCE_DIR}/Build ${joltphysics_BINARY_DIR})
endif()

# Force Jolt to use /MD(/d) even if its internal defaults change
if(MSVC AND TARGET Jolt)
    set_property(TARGET Jolt PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# =========== jolt fix ===========
if(MSVC AND TARGET Jolt)
    # Force dynamic runtime
    set_property(TARGET Jolt PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )

    # Stop Windows SDK warnings from breaking the build
    target_compile_options(Jolt PRIVATE
            /wd4865     # enum underlying type migration warning
            /Wv:18      # freeze warning set to VS2019-era
    )
endif()

# =========== Shader Compilation ===========

# Define the main source directory for shaders.
set(MAIN_SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

# Find all shader subdirectories by looking for varying.def.sc files.
file(GLOB_RECURSE VARYING_FILES "${MAIN_SHADER_SRC_DIR}/*/varying.def.sc")

# Create a list to hold all the compiled shader binary file paths.
set(SHADER_BINARIES "")

# Loop through each found varying file to compile its associated shaders.
foreach(VARYING_FILE ${VARYING_FILES})
    get_filename_component(SHADER_DIR ${VARYING_FILE} DIRECTORY)

    # Find the vertex and fragment shaders in the same directory.
    file(GLOB VERTEX_SHADER_LIST "${SHADER_DIR}/vs_*.sc")
    file(GLOB FRAGMENT_SHADER_LIST "${SHADER_DIR}/fs_*.sc")
    file(GLOB COMPUTE_SHADER_LIST  "${SHADER_DIR}/cs_*.sc")

    # MODIFICATION: Set the output directory to be the same as the source directory.
    set(SHADER_OUTPUT_DIR ${SHADER_DIR})

    # We still need the relative path for nice compile messages.
    file(RELATIVE_PATH REL_DIR ${MAIN_SHADER_SRC_DIR} ${SHADER_DIR})

    foreach(VERTEX_SHADER_FILE ${VERTEX_SHADER_LIST})
        get_filename_component(VS_NAME ${VERTEX_SHADER_FILE} NAME_WE)
        set(VS_OUTPUT_BINARY "${SHADER_OUTPUT_DIR}/${VS_NAME}.bin")

        add_custom_command(
                OUTPUT ${VS_OUTPUT_BINARY}
                # We use the target 'shaderc' here. CMake will automatically use the built executable path.
                COMMAND shaderc
                -f ${VERTEX_SHADER_FILE}
                -o ${VS_OUTPUT_BINARY}
                --type vertex
                --varyingdef ${VARYING_FILE} # <--- THIS FIXES YOUR ERROR
                --platform windows
                --profile vs_5_0
                -i ${bgfx_SOURCE_DIR}/bgfx/src
                DEPENDS ${VERTEX_SHADER_FILE} ${VARYING_FILE} shaderc # <--- Added shaderc dependency
                COMMENT "Compiling Vertex Shader: ${REL_DIR}/${VS_NAME}.sc"
        )
        list(APPEND SHADER_BINARIES ${VS_OUTPUT_BINARY})
    endforeach()

    foreach(FRAGMENT_SHADER_FILE ${FRAGMENT_SHADER_LIST})
        get_filename_component(FS_NAME ${FRAGMENT_SHADER_FILE} NAME_WE)
        set(FS_OUTPUT_BINARY "${SHADER_OUTPUT_DIR}/${FS_NAME}.bin")

        add_custom_command(
                OUTPUT ${FS_OUTPUT_BINARY}
                COMMAND shaderc
                -f ${FRAGMENT_SHADER_FILE}
                -o ${FS_OUTPUT_BINARY}
                --type fragment
                --varyingdef ${VARYING_FILE} # <--- THIS FIXES YOUR ERROR
                --platform windows
                --profile ps_5_0
                -i ${bgfx_SOURCE_DIR}/bgfx/src
                DEPENDS ${FRAGMENT_SHADER_FILE} ${VARYING_FILE} shaderc # <--- Added shaderc dependency
                COMMENT "Compiling Fragment Shader: ${REL_DIR}/${FS_NAME}.sc"
        )
        list(APPEND SHADER_BINARIES ${FS_OUTPUT_BINARY})
    endforeach()

    foreach(COMPUTE_SHADER_FILE ${COMPUTE_SHADER_LIST})
        get_filename_component(CS_NAME ${COMPUTE_SHADER_FILE} NAME_WE)
        set(CS_OUTPUT_BINARY "${SHADER_OUTPUT_DIR}/${CS_NAME}.bin")

        add_custom_command(
                OUTPUT ${CS_OUTPUT_BINARY}
                COMMAND shaderc
                -f ${COMPUTE_SHADER_FILE}
                -o ${CS_OUTPUT_BINARY}
                --type compute
                --varyingdef ${VARYING_FILE}
                --platform windows
                --profile cs_5_0
                -i ${bgfx_SOURCE_DIR}/bgfx/src
                DEPENDS ${COMPUTE_SHADER_FILE} ${VARYING_FILE} shaderc
                COMMENT "Compiling Compute Shader: ${REL_DIR}/${CS_NAME}.sc"
        )
        list(APPEND SHADER_BINARIES ${CS_OUTPUT_BINARY})
    endforeach()

endforeach()

# This creates the "Shaders" target that 'Game' is looking for.
add_custom_target(Shaders ALL DEPENDS ${SHADER_BINARIES})

# =========== Build Libraries ===========

# imgui
add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        #Backends
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# glad
set(GLAD_DIR "${CMAKE_SOURCE_DIR}/3rdparty/glad")
add_library(glad STATIC
        ${GLAD_DIR}/src/glad.c
)

# =========== Source Files ===========

file(GLOB_RECURSE SRC_FILES
        ${CMAKE_SOURCE_DIR}/engine/*.cpp
        ${CMAKE_SOURCE_DIR}/game/*.cpp
)

add_executable(Game main.cpp ${SRC_FILES})

# Make the Game executable depend on the shaders being compiled first.
add_dependencies(Game Shaders)


# =========== Include Directories ===========

target_include_directories(Game PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
        ${CMAKE_SOURCE_DIR}/game
        ${CMAKE_SOURCE_DIR}/3rdparty/stb/include
)

target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${glfw_SOURCE_DIR}/include
)

target_include_directories(glad PUBLIC
        ${GLAD_DIR}/include
)

# =========== Compile Definitions ===========
add_compile_definitions(GAME_ENGINE_ROOT="${CMAKE_SOURCE_DIR}")

# =========== Linking ===========
target_link_libraries(Game PRIVATE bgfx glfw assimp imgui glad Jolt)

# Ensure the app also uses the dynamic runtime (helps if you add more targets)
if(MSVC)
    set_property(TARGET Game PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    set_property(TARGET imgui PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    set_property(TARGET glad PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()