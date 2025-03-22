cmake_minimum_required(VERSION 3.10)
project(ConvCubes)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenGL
cmake_policy(SET CMP0072 OLD)
set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL REQUIRED)

# Find GLFW
set(GLFW_INCLUDE_DIR "/usr/include")
set(GLFW_LIBRARY "/usr/lib/x86_64-linux-gnu/libglfw.so")
include_directories(${GLFW_INCLUDE_DIR})
link_libraries(${GLFW_LIBRARY})

# Find GLM
find_package(glm REQUIRED)

# Find OpenCV
if(POLICY CMP0146)
    cmake_policy(SET CMP0146 OLD)
endif()
find_package(OpenCV REQUIRED)

# Add GLAD (Manually, since GLAD is usually included as source)
set(GLAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c)
set(GLAD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Collect all source files in src/
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# Add executable (including all source files)
add_executable(${PROJECT_NAME} ${SRC_FILES} ${GLAD_SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${OPENGL_INCLUDE_DIRS} 
    ${GLFW_INCLUDE_DIRS} 
    ${GLAD_INCLUDE_DIR} 
    ${GLM_INCLUDE_DIRS} 
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include   # Add custom headers directory
)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    OpenGL::GL 
    glfw 
    ${OpenCV_LIBS}
)
