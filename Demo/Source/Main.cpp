#define CATCH_CONFIG_RUNNER


#include <juce_gui_basics/juce_gui_basics.h>
#include <docks/docks.h>
#include <../../docks/tests/catch2.hpp>
#include "JuceHeader.h"

/**
 -------------------------------------------------------------
 ===================================
 MARK: - Test Component (TO show in view) -
 ===================================
 -------------------------------------------------------------
 */
class TestComponent : public juce::Component
{
public:
    TestComponent(const juce::String& name, const juce::Colour& color) : backgroundColor(color)
    {
        setName(name);
        setSize(800, 800);
    }
    
    void paint(juce::Graphics &g) override
    {
        g.fillAll(backgroundColor);
        g.setColour(juce::Colours::black);
        g.drawText(getName(), 0, 0, getWidth(), getHeight(), juce::Justification::centred);
    }
    
    juce::Colour backgroundColor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TestComponent)
};



/**
 -------------------------------------------------------------
 ===================================
 MARK: - Application -
 ===================================
 -------------------------------------------------------------
 */

class DockableWindowsV2Application  : public juce::JUCEApplication, public DockManager::Delegate
{
public:

    DockableWindowsV2Application() {}

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override             { return true; }


    void initialise (const juce::String& commandLine) override
    {
        DBG("Running...");
        int result = Catch::Session().run();
        DBG("Num Tests Failed: " << result);

        
        auto stream = juce::MemoryInputStream(BinaryData::layout_xml, BinaryData::layout_xmlSize, false);
        _windowManager.openLayout(stream);
        
//        _windowManager.create2Up("3Rows", getAvailableViews());
//        _windowManager.create3Up("3Up", getAvailableViews());
//        _windowManager.create4Up("4Up", getAvailableViews());
//        _windowManager.create2By2("2By2", getAvailableViews());
//        _windowManager.create3By3("3By3", getAvailableViews());
//        _windowManager.create2Rows("2Rows", getAvailableViews());
//        _windowManager.create3Rows("3Rows", getAvailableViews());
//        _windowManager.showOverlayWithText(true, "Hello World");
    }

    void shutdown() override
    {
        
    }


    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
    }
    
    const juce::StringArray getAvailableViews() const override
    {
        return {
            "Elements",
            "Canvases",
            "Canvas",
            "Details",
            "Cues",
            "Cue Lists",
            "Active",
            "Palette",
            "Chat",
            "Globals",
            "Messages",
            "State"
        };
    }
   
    
    std::shared_ptr<juce::Component> createView(const juce::String &nameOfViewToCreate) override
    {
        if (nameOfViewToCreate.isEmpty() || nameOfViewToCreate == "root") {return nullptr;}
        if (nameOfViewToCreate.contains("Cues"))
            return std::make_shared<TestComponent>(nameOfViewToCreate, juce::Colours::pink);
        else
            return std::make_shared<TestComponent>(nameOfViewToCreate, juce::Colours::lightgrey);
    }

    const juce::String getDefaultWindowName() const override
    {
        return "Default Window Name";
    }
    
    std::shared_ptr<juce::MenuBarComponent> getMenuForWindow(const juce::String &windowName) override
    {
        return std::make_shared<juce::MenuBarComponent>();
    }
    
    std::shared_ptr<juce::Component> getFooterForWindow(const juce::String &windowName) override
    {
        return std::make_shared<juce::Component>();
    }
    
private:
    
    DockManager _windowManager {*this};
};


START_JUCE_APPLICATION (DockableWindowsV2Application)
