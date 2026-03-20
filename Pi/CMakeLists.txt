cmake_minimum_required(VERSION 3.10)
project(dryer-eurorack VERSION 1.0 LANGUAGES CXX)

# C++17 standard required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")

# Find required packages
find_package(SDL2 REQUIRED)

# Include directories
include_directories(${SDL2_INCLUDE_DIRS})

# Source files
set(SOURCES
    dryer-main.cpp
    dryer-physics.cpp
    dryer-hardware.cpp
    dryer-renderer.cpp
)

# Headers
set(HEADERS
    pins.h
    dryer-physics.h
    dryer-hardware.h
    dryer-renderer.h
)

# Create executable
add_executable(dryer ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(dryer 
    ${SDL2_LIBRARIES}
    gpiod          # libgpiod for GPIO
    pthread        # For threading (MIDI note off timing)
    m             # Math library
)

# Install target
install(TARGETS dryer DESTINATION /usr/local/bin)

# Print configuration
message(STATUS "==============================================")
message(STATUS "Dryer Eurorack Module - Build Configuration")
message(STATUS "==============================================")
message(STATUS "SDL2 include: ${SDL2_INCLUDE_DIRS}")
message(STATUS "SDL2 libs: ${SDL2_LIBRARIES}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "==============================================")
