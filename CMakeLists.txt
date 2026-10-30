cmake_minimum_required(VERSION 3.22)
project(LifeSimulation VERSION 0.2.0 LANGUAGES CXX)

option(LIFE_BUILD_APP "Build the SFML interactive application" ON)
option(LIFE_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(life_core
    src/life/Genome.cpp
    src/life/NutrientField.cpp
    src/life/Cell.cpp
    src/life/Organism.cpp
    src/life/Simulation.cpp
)
target_include_directories(life_core PUBLIC include)
target_compile_features(life_core PUBLIC cxx_std_20)
set_target_properties(life_core PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(life_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(life_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(LIFE_ENABLE_SANITIZERS)
        target_compile_options(life_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(life_core PUBLIC -fsanitize=address,undefined)
    endif()
endif()

if(LIFE_BUILD_APP)
    find_package(SFML 3 REQUIRED COMPONENTS Graphics)
    add_executable(LifeSimulation src/main.cpp src/life/Renderer.cpp)
    target_link_libraries(LifeSimulation PRIVATE life_core SFML::Graphics)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(life_tests tests/SimulationTests.cpp)
    target_link_libraries(life_tests PRIVATE life_core)
    add_test(NAME life_tests COMMAND life_tests)
endif()
