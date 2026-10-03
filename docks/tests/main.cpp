#define CATCH_CONFIG_RUNNER
#include <docks/docks.h>
#include "catch2.hpp"

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    return Catch::Session().run(argc, argv);
}
