cmake_minimum_required(VERSION 3.12)
project(phaistos VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Add compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-c++98-compat)
elseif(MSVC)
  add_compile_options(/W4)
endif()

# Find required libraries
find_package(Threads REQUIRED)

# Source files
set(SOURCES
    src/logger.cpp
    src/main.cpp
    src/cpu.cpp
    src/optimizer.cpp
    src/parser.cpp
    src/sequence_generator.cpp
    src/solution_formatter.cpp
    src/transformation_cache.cpp
    src/value.cpp
    src/verification_engine.cpp
    src/fake6502/fake6502.cpp
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Create executable
add_executable(phaistos ${SOURCES})

# Link libraries
target_link_libraries(phaistos PRIVATE Threads::Threads)

# Install
install(TARGETS phaistos DESTINATION bin)

# Testing
option(BUILD_TESTS "Build test programs" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
    else()
        message(STATUS "Doxygen not found, documentation will not be built")
    endif()
endif()
