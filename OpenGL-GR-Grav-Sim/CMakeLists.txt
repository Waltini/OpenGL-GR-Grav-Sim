cmake_minimum_required(VERSION 3.15)
project(OpenGL_GR_Grav_Sim LANGUAGES CXX)

# ------------------------------------------------------------
# Output folders (clean, Visual Studio-safe)
# ------------------------------------------------------------
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)

foreach(OUTPUTCONFIG DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/dist)
endforeach()

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable folders in VS
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ------------------------------------------------------------
# Include directories
# ------------------------------------------------------------
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/libraries
    ${PROJECT_SOURCE_DIR}/libraries/glad/include
    ${PROJECT_SOURCE_DIR}/libraries/GLFW
    ${PROJECT_SOURCE_DIR}/libraries/glm
    ${PROJECT_SOURCE_DIR}/libraries/imgui
    ${PROJECT_SOURCE_DIR}/libraries/imgui/backends
)

# ------------------------------------------------------------
# Main source files
# ------------------------------------------------------------
file(GLOB_RECURSE SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/libraries/imgui/*.cpp
    ${PROJECT_SOURCE_DIR}/libraries/imgui/backends/*.cpp
)

# ------------------------------------------------------------
# ✅ GLAD as static library
# ------------------------------------------------------------
add_library(glad STATIC
    ${PROJECT_SOURCE_DIR}/libraries/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    ${PROJECT_SOURCE_DIR}/libraries/glad/include
)

set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)

# ------------------------------------------------------------
# ✅ GLM header-only interface library
# ------------------------------------------------------------
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
    ${PROJECT_SOURCE_DIR}/libraries/glm
)

# ------------------------------------------------------------
# ✅ Executable
# ------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SRC_FILES})

# Force Visual Studio to ALWAYS build glad first
add_dependencies(${PROJECT_NAME} glad)

# ------------------------------------------------------------
# Libraries
# ------------------------------------------------------------
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glad          # MUST be first
        glfw
        glm
        OpenGL::GL
)

# ------------------------------------------------------------
# Copy resources and shaders on build
# ------------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/shaders" $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/resources" $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Build complete. Executable is in dist/ folder."
)