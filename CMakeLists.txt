#=================================#
# Project configuration
#
cmake_minimum_required(VERSION 3.16) # Qt 6 requires CMake >= 3.16
set(PROJNAME "discovering-path-tracer")
project(${PROJNAME} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)  
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
# GLSL to SPIR-V custom build
#
# more than one file can be given: _compile_GLSL("GLSL_mesh.vert;GLSL_mesh.frag" "GLSL_mesh.spv" GLSL_SOURCES)
# the SPIR-V validator is fine as long as files are for different pipeline stages (entry points still need to be main())
#_compile_GLSL(<source(s)> <target spv> <LIST where files are appended>)
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


UNSET(GLSL_SOURCES)
UNSET(SPV_OUTPUT)
file(GLOB_RECURSE GLSL_HEADER_FILES "src/shaders/*.h")
file(GLOB_RECURSE GLSL_SOURCE_FILES "src/shaders/*.glsl")

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME_WE)
    message(STATUS "FILENAME: ${FILE_NAME}")

    # _compile_GLSL(${GLSL} "src/shaders/${FILE_NAME}.spv" GLSL_SOURCES SPV_OUTPUT)
    # #             ${_SOURCE} ${_OUTPUT} "-g"            ${SOURCE_LIST} ${OUTPUT_LIST}

    set(_SOURCE ${GLSL})
    set(_OUTPUT "${CMAKE_SOURCE_DIR}/src/shaders/${FILE_NAME}.spv")
    set(_FLAGS "")

    message(STATUS "_SOURCE: ${_SOURCE}")
    message(STATUS "_OUTPUT: ${_OUTPUT}")

    list(APPEND ${GLSL_SOURCES} ${_SOURCE})
    list(APPEND ${SPV_OUTPUT} ${_OUTPUT})

    if(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
        set(_COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} ${_SOURCE} -o ${_OUTPUT})
        message(STATUS "_COMMAND SET")
        add_custom_command(
            OUTPUT ${_OUTPUT}
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} ${_SOURCE} -o ${_OUTPUT}
            DEPENDS ${_SOURCE}
            COMMENT "Compiling GLSL shader: ${_SOURCE} -> ${_OUTPUT}"
        )
        message(STATUS "AFTER CUSTOM COMMAND")
    else(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
        MESSAGE(WARNING "could not find Vulkan_GLSLANG_VALIDATOR_EXECUTABLE to compile shaders")
    endif(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)  
endforeach(GLSL)

list(APPEND GLSL_SOURCES ${GLSL_HEADER_FILES})
source_group("Shader Files" FILES ${GLSL_SOURCES})

add_custom_target(compile_shaders DEPENDS ${_OUTPUT})

#=================================#
# Executable
#
add_executable(${PROJNAME} ${SOURCE_FILES} ${GLSL_SOURCES})

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

# #=================================#
# # Resources (shaders)
# #
# set_source_files_properties("src/shaders/color_frag.spv"
#     PROPERTIES QT_RESOURCE_ALIAS "color_frag.spv"
# )
# set_source_files_properties("src/shaders/color_vert.spv"
#     PROPERTIES QT_RESOURCE_ALIAS "color_vert.spv"
# )   
# set(RESOURCE_FILES
#     "src/shaders/color_frag.spv"
#     "src/shaders/color_vert.spv"
# )
# qt_add_resources(${PROJNAME} "discovering-path-tracer"
#     PREFIX
#         "/"
#     FILES
#         ${RESOURCE_FILES}
# )

#===================================#
# Automatically discover shaders
#
file(GLOB RESOURCE_FILES "src/shaders/*.spv")

# Set QT resource aliases automatically
foreach(FILE ${RESOURCE_FILES})
    get_filename_component(FILENAME ${FILE} NAME)
    set_source_files_properties(${FILE} PROPERTIES QT_RESOURCE_ALIAS ${FILENAME})
endforeach()

# Generate QRC file dynamically
set(QRC_FILE "${CMAKE_SOURCE_DIR}/src/main/shaders.qrc")
configure_file("${CMAKE_SOURCE_DIR}/src/main/shaders.qrc.in" "${QRC_FILE}" @ONLY)

# Add the resources to the project
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

#=================================#
# For convenient #include of header files
#
target_include_directories(${PROJNAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

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
    WIN32_EXECUTABLE FALSE 
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