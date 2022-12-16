# Specify the minimum version of CMake we require. As we don't do anything unusual, use the minimum 
# version JUCE itself wants.
cmake_minimum_required(VERSION 3.15)

# Specify the version for the docks project. For now, let's use 0.1.0
project(docks VERSION 0.1.0 LANGUAGES CXX)

# Assume JUCE has been cloned into a subdirectory 
# If you have the JUCE source tree somewhere else, set the option to the path you want!
option(DOCKS_JUCE_DIR "Set this to the JUCE source directory if not ./JUCE in this directory")
if(DOCKS_JUCE_DIR)
add_subdirectory(${DOCKS_JUCE_DIR})
else()
add_subdirectory(JUCE)
endif()

# Now define the docks module itself - this does not create a compile target but rather allows the Demo
# program to link to it which will include the cpp file of the module in the JUCE way of doing things
juce_add_module(docks)

# This is the definition of the GUI Demo app
juce_add_binary_data(Resources SOURCES "${CMAKE_CURRENT_LIST_DIR}/Demo/layout.xml" "${CMAKE_CURRENT_LIST_DIR}/docks/images/LockOn.png")
juce_add_gui_app(Demo)
target_sources(Demo PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Demo/Source/Main.cpp")
target_link_libraries(Demo PRIVATE 
    juce::juce_core
    juce::juce_gui_basics
    docks
    Resources
    )
juce_generate_juce_header(Demo)
