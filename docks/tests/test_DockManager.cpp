
#include "catch2.hpp"
#include "../source/DockManager.h"

/// Mock Delegate
class TestManagerDelegate : public DockManager::Delegate
{
public:
    const juce::StringArray getAvailableViews() const override { return {"Elements", "Canvas", "Cues", "Palette", "CueLists", "ElementLists", "Globals", "Monitors", "State"}; }
    std::shared_ptr<juce::Component> createView(const juce::String &nameOfViewToCreate) override { return nullptr; }
    const juce::String getDefaultWindowName() const override {return "Window";}
};


/// Testing class with access to DockManager internals
class test_DockManager : public DockManager
{
public:
    test_DockManager(DockManager::Delegate& delegate) : DockManager(delegate) {}
    void printTree() {DockManager::printTree();}

};


/**
 ===================================
 MARK: - Construction/Destruction -
 ===================================
 */

TEST_CASE("Check Manager Initialized without throwing")
{
    auto delegate = TestManagerDelegate();
    CHECK_NOTHROW(DockManager(delegate));
}


/// To Be fair I don't quite know if this does anything;
TEST_CASE("Check Manager Destructor")
{
    auto delegate = TestManagerDelegate();
    auto manager = std::make_unique<DockManager>(delegate);
    CHECK(manager != nullptr);
    manager.reset();
    CHECK(manager == nullptr);
}


/**
 ===================================
 MARK: - Windows -
 ===================================
 */

TEST_CASE("addNewWindow")
{

}


TEST_CASE("removeWindow")
{
    
}


TEST_CASE("closeAllWindows")
{
    
}


/**
 ===================================
 MARK: - Utility -
 ===================================
 */

TEST_CASE("getTree")
{
    auto delegate = TestManagerDelegate();
    auto manager = test_DockManager(delegate);
    
}
