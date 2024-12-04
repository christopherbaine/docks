# Specify the minimum version of CMake we require. As we don't do anything unusual, use the minimum 
# version JUCE itself wants.
cmake_minimum_required(VERSION 3.15)

# Specify the version for the docks project. For now, let's use 0.1.0
project(docks VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY    https://github.com/juce-framework/JUCE.git
    GIT_TAG     v0.8.3
    GIT_PROGRESS TRUE
)


FetchContent_MakeAvailable(juce)

set(TARGET_NAME docks)
add_library(${TARGET_NAME})
add_library(${TARGET_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})

file (GLOB_RECURSE BINARY_FILES "docks/images/*.*")
juce_add_binary_data(docks_binarydata SOURCES ${BINARY_FILES})

target_link_libraries(${TARGET_NAME} 
                        PRIVATE 
                            juce::juce_core
                            juce::juce_gui_basics
                            docks_binarydata

)  


file (GLOB_RECURSE HEADERS "docks/*.h")
file (GLOB_RECURSE SOURCES "docks/*.cpp")
target_sources(${TARGET_NAME}  PRIVATE ${HEADERS} ${SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE "docks")
