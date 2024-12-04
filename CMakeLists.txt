# Specify the minimum version of CMake we require. As we don't do anything unusual, use the minimum 
# version JUCE itself wants.
cmake_minimum_required(VERSION 3.15)

# Specify the version for the docks project. For now, let's use 0.1.0
project(docks VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY    https://github.com/juce-framework/JUCE.git
    GIT_TAG     5737c42ccf0a5c29abea4f650c34b53f8ae37c32
    GIT_PROGRESS TRUE
)


FetchContent_MakeAvailable(juce)

set(TARGET_NAME docks)
add_library(${TARGET_NAME})


target_link_libraries(${TARGET_NAME} 
                        PRIVATE 
                            juce::juce_core
                            juce::juce_gui_basics
)  


file (GLOB_RECURSE HEADERS "docks/*.h")
file (GLOB_RECURSE SOURCES "docks/*.cpp")
target_sources(${TARGET_NAME}  PRIVATE ${HEADERS} ${SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE "docks")
