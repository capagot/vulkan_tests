cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(VULKAN_TEST LANGUAGES CXX)

###############################################################################
# Enforce c++11 standard
###############################################################################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_STANDARD 11)          # enforces c++11
set(CMAKE_CXX_STANDARD_REQUIRED ON) # does not fallback if the compiler does not cupport c++11
set(CMAKE_CXX_EXTENSIONS OFF)       # sets -std=c++11 instead of -std=g++11

###############################################################################
# Set output directories
###############################################################################

#set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

###############################################################################
# Select between Release and Debug
###############################################################################

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
else(CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-DNDEBUG)
endif()

###############################################################################
# Find packages: Vulkan
###############################################################################

find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED) # needed to find glfw3
pkg_search_module(GLFW REQUIRED glfw3)

###############################################################################
# Prepares the application
###############################################################################

add_executable(vulkan_test
               ${PROJECT_SOURCE_DIR}/main.cpp)

target_include_directories(vulkan_test
                           PUBLIC
                           ${GLFW_INCLUDE_DIRS}
                           "/home/christian/work/common/glm-0.9.9.3"
                           ${Vulkan_INCLUDE_DIRS})

target_link_libraries(vulkan_test
                      ${Vulkan_LIBRARIES}
                      ${GLFW_LIBRARIES}
                      "dl"
                      "X11"
                      "pthread"
                      "Xxf86vm"
                      "Xrandr"
                      "Xinerama"
                      "Xcursor")

