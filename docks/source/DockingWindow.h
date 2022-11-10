#pragma once

#include <juce_gui_basics/juce_gui_basics.h> 
#include "DockManager.h"
#include "DockManagerData.h"


class DockManager;
class DockManagerData;
class DockingComponent;




/**
 -------------------------------------------------------------
 ===================================
 MARK: - Window Drop Component -
 ===================================
 -------------------------------------------------------------
 */

class WindowDropHandle : public juce::Component
{
public:
    WindowDropHandle(); 
    void paint(juce::Graphics& g) override;
    
    void setDropLocation(DropLocation type);
    
private:
    
    DropLocation _type = DropLocation::tabs;
};


/**
 -------------------------------------------------------------
 ===================================
 MARK: - Window Component -
 The root Component for the window
 ===================================
 -------------------------------------------------------------
 */

class WindowComponent : public juce::Component, public juce::ValueTree::Listener, public juce::DragAndDropTarget
{
public:
    WindowComponent(DockingWindow& window, DockManager& manager, DockManagerData& data, const juce::ValueTree& tree);
    ~WindowComponent();

    /// Drop Handle
    void showDropHandleAt(const juce::String& uuid, int index, DropLocation type);
    void hideDropHandle();
    
    /// Refresh
    void refresh();
    
    /// Layout Did Load
    void layoutDidLoad();
    
    /// Display Name
    void resetAllDisplayNames();
    
    /// Overlay
    void showOverlay(bool show, const juce::String& textToShow);

private:
    /// Setup
    void setupMenu();
    void setupFooter(); 
    
    /// Component Overrides
    void resized() override;
    void paint(juce::Graphics &g) override;
    
    /// Value Tree Listener
    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    void valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;
    
    /// Drag and Drop Target
    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
    void itemDragEnter(const SourceDetails &dragSourceDetails) override;
    void itemDragExit(const SourceDetails &dragSourceDetails) override;
    void itemDropped(const SourceDetails &dragSourceDetails) override;
    void itemDragMove(const SourceDetails &dragSourceDetails) override;
    
    /// Get Drag Location
    const DropLocation getDragLocation(const juce::Point<int>& position) const;
    
private:
    
    /// Window Handle
    DockingWindow& _window;
    
    /// Dock Manager
    DockManager& _manager;
    
    /// Data
    DockManagerData& _data;
    
    /// Value Tree
    juce::ValueTree _tree;

    /// Docking Component
    std::unique_ptr<DockingComponent> _dockingComponent;
    std::unique_ptr<juce::ImageButton> _lockedButton;
    
    /// Handle
    WindowDropHandle _dropHandle;
    
    /// Menu
    std::shared_ptr<juce::MenuBarComponent> _menuComponent = nullptr;
    
    /// Footer
    std::shared_ptr<juce::Component> _footerComponent = nullptr;
    
    /// Overlay
    std::unique_ptr<juce::Component> _overlay = nullptr; 

    /// Utility
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WindowComponent)
};






/**
 -------------------------------------------------------------
 ===================================
 MARK: - Docking Window -
 ===================================
 -------------------------------------------------------------
 */
class DockingWindow : public juce::DocumentWindow
{
public:
    enum ColourIds
    {
        backgroundColourId = 10001
    };
    
    DockingWindow(DockManager& manager, DockManagerData& data, const juce::ValueTree& tree);
    ~DockingWindow();
    
    void layoutDidLoad() {_rootComponent.layoutDidLoad();}
    void resetAllDisplayNames() {_rootComponent.resetAllDisplayNames();}
    
    /// Overlay
    void showOverlay(bool show, const juce::String& textToShow);
private:
    /// Window Overrides
    void closeButtonPressed() override;
    void minimiseButtonPressed() override;
    void maximiseButtonPressed() override;
    void moved() override;
    void resized() override;

    /// Checks
    void checkWindowSize();
    
private:
    
    /// Dock Manager
    DockManager& _manager;
    
    /// Data
    DockManagerData& _data;
    
    /// Value Tree
    juce::ValueTree _tree;

    /// Docking Component
    WindowComponent _rootComponent;
    
    /// Utility
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DockingWindow)
};


