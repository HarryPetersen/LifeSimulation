
cmake_minimum_required(VERSION 3.22)

project(LifeSimulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find installed SFML 3
find_package(SFML 3 REQUIRED COMPONENTS Graphics)

# Create the executable
add_executable(LifeSimulation
    src/main.cpp
)

# Include our own header files
target_include_directories(LifeSimulation PRIVATE include)

# Link SFML
target_link_libraries(LifeSimulation PRIVATE SFML::Graphics)
