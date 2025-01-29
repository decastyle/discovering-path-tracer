cmake_minimum_required(VERSION 3.16) # Qt 6 requires CMake >= 3.16
set(PROJNAME "discovering-path-tracer")
project(${PROJNAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DQT_QML_DEBUG)
set(ENV{QT_LOGGING_RULES} "qt.*=true")

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

file(GLOB_RECURSE SOURCE_FILES # idk what is the best practice for src file management, so I'll do this for now
    src/*.cpp
    src/*.c
    src/*.hpp
    src/*.h
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(${PROJNAME} ${SOURCE_FILES})

# Qt configuration
find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Gui
    Widgets
    Concurrent
)
target_link_libraries(${PROJNAME} PRIVATE 
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
)
qt_standard_project_setup()


# Resources
set_source_files_properties("src/shaders/color_frag.spv"
    PROPERTIES QT_RESOURCE_ALIAS "color_frag.spv"
)

set_source_files_properties("src/shaders/color_vert.spv"
    PROPERTIES QT_RESOURCE_ALIAS "color_vert.spv"
)   

set(RESOURCE_FILES
    "src/shaders/color_frag.spv"
    "src/shaders/color_vert.spv"
)
qt_add_resources(${PROJNAME} "discovering-path-tracer"
    PREFIX
        "/"
    FILES
        ${RESOURCE_FILES}
)


# Vulkan configuration
find_package(Vulkan REQUIRED)
target_include_directories(${PROJNAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJNAME} PRIVATE ${Vulkan_LIBRARIES})

# Optionally, set Vulkan SDK path if not automatically found
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please set Vulkan_INCLUDE_DIRS and Vulkan_LIBRARIES manually.")
endif()

target_include_directories(${PROJNAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Platform-specific configuration
if(WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
    add_definitions(-DVK_USE_PLATFORM_MACOS_MVK)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
endif()

set_target_properties(${PROJNAME} PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)   

# Add compiler warnings 
if(MSVC)
    target_compile_options(${PROJNAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJNAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
