cmake_minimum_required(VERSION 3.13)

project(physics_sandbox)


find_package(OpenGL REQUIRED)

set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")
set(INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/assimp/include" "${CMAKE_SOURCE_DIR}/glfw/include")
set(CMAKE_CXX_STANDARD 17)
include_directories(${OPENGL_INCLUDE_DIRS} ${VENDOR_DIR} ${INCLUDE_DIRS})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

add_subdirectory(glfw)
add_subdirectory(assimp)

file(GLOB_RECURSE SRC_CXX_FILES "${SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE SRC_C_FILES "${SOURCE_DIR}/*.c")

if(MSVC)
SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup" )
endif()

add_executable(box ${SRC_CXX_FILES} ${SRC_C_FILES})
target_link_libraries(box ${OPENGL_LIBRARIES} glfw assimp)

if( MSVC )
        set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT)
endif()