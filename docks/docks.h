#ifndef DOCKS_H
#define DOCKS_H

/**

BEGIN_JUCE_MODULE_DECLARATION

ID:               docks
vendor:           Christopher Baine
version:          0.0.1
name:             docks
description:  Docks is a module which allows for docking/exploding windows with custom views
license:         MIT
 
dependencies:     juce_core, juce_events, juce_gui_basics, juce_gui_extra, juce_graphics, juce_data_structures
minimumCppStandard: 17

END_JUCE_MODULE_DECLARATION
*/


#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_data_structures/juce_data_structures.h>


#include "source/DockManager.h"
#include "source/DockManagerData.h"
#include "source/DockingWindow.h"
#include "source/DockingComponent.h"
#include "source/HeaderComponent.h"

//#include "tests/catch2.hpp"

#endif /// End Docks

