#=================================#
# Project configuration
#
cmake_minimum_required(VERSION 3.16) # Qt 6 requires CMake >= 3.16
set(PROJNAME "discovering-path-tracer")
project(${PROJNAME} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20) # For designated initializers  
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(VULKAN_TARGET_ENV vulkan1.4)

#=================================#
# Flags for debugging (qDebug, no optimization)
#
add_definitions(-DQT_QML_DEBUG)
set(ENV{QT_LOGGING_RULES} "qt.*=true")

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

#=================================#
# Source files for this project
#
file(GLOB_RECURSE SOURCE_FILES # idk what is the best practice for src file management, so I'll do this for now
    src/*.cpp
    src/*.c
    src/*.hpp
    src/*.h
)
#=================================#
# Automatic Qt resource management
#
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

#=================================#
# Executable
#
add_executable(${PROJNAME} ${SOURCE_FILES})

#=================================#
# Qt configuration
#
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


#=================================#
# Vulkan binaries from VULKAN_SDK environment variable (FindVulkan.cmake)
#
if(WIN32)
    find_path(Vulkan_INCLUDE_DIR
        NAMES vulkan/vulkan.h
        HINTS
        "$ENV{VULKAN_SDK}/Include"
        )
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        find_library(Vulkan_LIBRARY
        NAMES vulkan-1
        HINTS
            "$ENV{VULKAN_SDK}/Lib"
            "$ENV{VULKAN_SDK}/Bin"
        )
        find_program(Vulkan_GLSLC_EXECUTABLE
        NAMES glslc
        HINTS
            "$ENV{VULKAN_SDK}/Bin"
        )
        find_program(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE
        NAMES glslangValidator
        HINTS
            "$ENV{VULKAN_SDK}/Bin"
        )
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        find_library(Vulkan_LIBRARY
        NAMES vulkan-1
        HINTS
            "$ENV{VULKAN_SDK}/Lib32"
            "$ENV{VULKAN_SDK}/Bin32"
        )
        find_program(Vulkan_GLSLC_EXECUTABLE
        NAMES glslc
        HINTS
            "$ENV{VULKAN_SDK}/Bin32"
        )
        find_program(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE
        NAMES glslangValidator
        HINTS
            "$ENV{VULKAN_SDK}/Bin32"
        )
    endif()
    else()
    find_path(Vulkan_INCLUDE_DIR
        NAMES vulkan/vulkan.h
        HINTS "$ENV{VULKAN_SDK}/include")
    find_library(Vulkan_LIBRARY
        NAMES vulkan
        HINTS "$ENV{VULKAN_SDK}/lib")
    find_program(Vulkan_GLSLC_EXECUTABLE
        NAMES glslc
        HINTS "$ENV{VULKAN_SDK}/bin")
    find_program(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE
        NAMES glslangValidator
        HINTS "$ENV{VULKAN_SDK}/bin")
endif()

#=================================#
# GLSL to SPIR-V compilation on configuration step
#
file(GLOB_RECURSE GLSL_FILES # i can't stop lol
    src/shaders/*.frag
    src/shaders/*.vert
    src/shaders/*.comp
    src/shaders/*.glsl
    src/shaders/*.rgen
)

set(SPV_FILES "")

foreach(GLSL_FILE IN LISTS GLSL_FILES)
    get_filename_component(FILE_NAME ${GLSL_FILE} NAME_WE)
    get_filename_component(FILE_DIR ${GLSL_FILE} DIRECTORY)
    set(SPV_FILE "${CMAKE_SOURCE_DIR}/src/shaders/${FILE_NAME}.spv")
    message(STATUS "${SPV_FILE}")

    message(STATUS "Compiling ${GLSL_FILE} with:")
    message(STATUS "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V --target-env ${VULKAN_TARGET_ENV} --verbose ${GLSL_FILE} -o ${SPV_FILE}")


    # I love you glslangValidator
    execute_process(
        COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} 
        -V 
        --target-env ${VULKAN_TARGET_ENV}
    
        ${GLSL_FILE} -o ${SPV_FILE}
        RESULT_VARIABLE COMPILE_RESULT
        OUTPUT_VARIABLE COMPILE_OUTPUT
        ERROR_VARIABLE COMPILE_ERROR
        COMMAND_ECHO STDOUT
    )

    if(NOT COMPILE_RESULT EQUAL 0)
        message(FATAL_ERROR "Shader compilation failed for ${GLSL_FILE}:\n${COMPILE_OUTPUT}\n${COMPILE_ERROR}")
    else()
        message(STATUS "Compiled ${GLSL_FILE} -> ${SPV_FILE}\n${COMPILE_OUTPUT}")
    endif()


    list(APPEND SPV_FILES ${SPV_FILE})
endforeach()

#===================================#
# Automatically discover shaders
#
file(GLOB RESOURCE_FILES "src/shaders/*.spv")

foreach(FILE ${RESOURCE_FILES})
    get_filename_component(FILENAME ${FILE} NAME)
    set_source_files_properties(${FILE} PROPERTIES QT_RESOURCE_ALIAS ${FILENAME})
endforeach()


# Generate QRC file dynamically (i couldn't come up with more shitty way to do this okay)
set(QRC_FILE "${CMAKE_SOURCE_DIR}/src/main/shaders.qrc")
file(WRITE ${QRC_FILE} "<?xml version=\"1.0\"?>\n")
file(APPEND ${QRC_FILE} "<!DOCTYPE RCC><RCC version=\"1.0\">\n")
file(APPEND ${QRC_FILE} "  <qresource>\n")

foreach(SHADER_FILE ${RESOURCE_FILES})
    get_filename_component(FILE_NAME ${SHADER_FILE} NAME)
    file(APPEND ${QRC_FILE} "    <file alias=\"${FILE_NAME}\">../shaders/${FILE_NAME}</file>\n")
endforeach()

file(APPEND ${QRC_FILE} "  </qresource>\n")
file(APPEND ${QRC_FILE} "</RCC>\n")


qt_add_resources(${PROJNAME} "discovering-path-tracer"
    PREFIX "/"
    FILES ${RESOURCE_FILES}
)

#=================================#
# Vulkan configuration
#
find_package(Vulkan REQUIRED)
target_include_directories(${PROJNAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJNAME} PRIVATE ${Vulkan_LIBRARIES})

#=================================#
# Optionally, set Vulkan SDK path if not automatically found
#
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please set Vulkan_INCLUDE_DIRS and Vulkan_LIBRARIES manually.")
endif()

# #=================================#
# # volk configuration
# #
# add_subdirectory(external/volk)

# target_link_libraries(${PROJNAME} PRIVATE volk::volk) 

#=================================#
# For convenient #include of header files
#
target_include_directories(${PROJNAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(${PROJNAME} PRIVATE ${CMAKE_SOURCE_DIR}/external)

#=================================#
# Platform-specific configuration
#
if(WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
    add_definitions(-DVK_USE_PLATFORM_MACOS_MVK)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
endif()

set_target_properties(${PROJNAME} PROPERTIES
    # WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)   

#=================================#
# Add compiler warnings 
#
if(MSVC)
    target_compile_options(${PROJNAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJNAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()