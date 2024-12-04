#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "HeaderComponent.h"
#include "DockManagerData.h"


/**
 GOALS:
 - Get Tab rearranging working
 - Get Tab Animation Working
 */

class DockManager;
class DockManagerData;
class WindowComponent;



/**
 -------------------------------------------------------------
 ===================================
 MARK: - Docking Component -
 ===================================
 -------------------------------------------------------------
 */

class DockingComponent : public juce::Component, private juce::ValueTree::Listener, public juce::DragAndDropTarget, public juce::DragAndDropContainer
{
    friend class HeaderComponent;
    
public:
    
    enum ColourIds
    {
        backgroundColourId = 10001,      /// Background
        tabColorId = 10002,              /// Tab Background color
        tabSelectedColorId = 10003,      /// Tab selected color
        tabBorderColorId = 10004,        /// Tab border Color
        headerBackgroundColorId = 10005, /// Header (behind tab) background color
        textColorId = 10006,             /// Text color (for header/tabs)
        draggingColorId = 10007,         /// Dragging Color (for docking frame)
        focusOutlineId = 10008           /// Focus Outline Color
    };
    
    DockingComponent(DockManager& manager, DockManagerData& data, const juce::ValueTree& tree);
    ~DockingComponent() override;

    /// Getters
    const bool hasSubItems() const;
    const bool isTabs() const;
    const juce::String getName() const;
    const juce::String getUuid() const;
    const bool shouldShowHeader() const;
    const juce::Rectangle<int> getBoundsForSubview(const juce::String& uuid, int index) const;
    
    /// Check Will Disappear
    void checkWillDisappear();
    
    /// Loaded Layout
    void layoutDidLoad();
    
    /// Reset Header
    void resetDisplayName(); 
private:

    const DropLocation getDragLocation(const juce::Point<int> position) const;
 

    /// Component Overrides
    void paint(juce::Graphics &g) override;
    void resized() override;
    void lookAndFeelChanged() override;
    
    /// Setup
    void setupWithTree();
    void setupHeader();
    void selectedTabDidChange();
    void setupResizerBars();
    void setupView();
    void setupKeyboardFocus();
    
    /// Value Tree Listener
    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    void valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;
    
    /// Focus
    void focusOfChildComponentChanged(FocusChangeType cause) override;
    
    /// Drag and Drop Target
    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
    void itemDropped(const SourceDetails &dragSourceDetails) override;
    void itemDragEnter(const SourceDetails &dragSourceDetails) override;
    void itemDragExit(const SourceDetails &dragSourceDetails) override;
    void itemDragMove(const SourceDetails &dragSourceDetails) override;
    
    /// Drag and Drop Container
    void dragOperationStarted(const juce::DragAndDropTarget::SourceDetails& details) override;
    void dragOperationEnded(const juce::DragAndDropTarget::SourceDetails& details) override;
    
    /// Resizer Utility
    void resizerDidDrag(juce::Point<float> delta, int index);
    void resizerMouseUp();
    void checkViewsShouldExist();
    void setViewsMayDisappear();
    void resizeParent();
    
    /// Keyboard
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyPressed_tabs(const juce::KeyPress& key);
    bool selectTab(int index);
    bool selectNextTab();
    bool selectPreviousTab();
    bool changeFocusTo(const juce::KeyPress& key);
    
private:
    
    const int _headerHeight = 25;
    const int _viewHitSize = 50;
    const int _parentHitSize = 25;
    const int _rootHitSize = 10;
    const int _resizerSize = 5;
    const int _minimumSize = 100;
    
    /// Dock Manager
    DockManager& _manager;
    
    /// Data
    DockManagerData& _data;
    
    /// Value Tree
    juce::ValueTree _tree;
    
    /// Components Parts
    juce::Array<std::shared_ptr<DockingComponent>> _components;
    juce::Array<std::shared_ptr<juce::Component>> _resizerBars;
    
    /// Header
    std::unique_ptr<HeaderComponent> _header = nullptr;
    std::unique_ptr<juce::Component> _floater = nullptr;
    std::shared_ptr<juce::Component> _view = nullptr;
    
    /// Drag and Drop
    bool _didDropOnView = false;
    
    /// Mouse
    bool _didDrag = false;
    bool _willDisappear = false;
    
    /// Utility
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DockingComponent)
};
