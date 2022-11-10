#pragma once

/// includes
#include <juce_gui_basics/juce_gui_basics.h>


/// Forward Definitions
class DockManager;
class DockManagerData;




/**
 -------------------------------------------------------------
 ===================================
 MARK: - Tab Component -
 ===================================
 -------------------------------------------------------------
 */


class TabComponent : public juce::Component, private juce::ValueTree::Listener
{
public:
    TabComponent(DockManager& manager, DockManagerData& data, const juce::ValueTree& tree);
    ~TabComponent();

    /// Getters
    const juce::String getUuid() const;
    const juce::String getDisplayName() const;
    const bool getSelected() const;
    const juce::ValueTree getTree() const; 
private:
    
    /// Component Overrides
    void resized() override;
    void paint(juce::Graphics &g) override;

    /// Value Tree Listener
    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    void valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;
    
    
private:
    /// Dock Manager
    DockManager& _manager;
    
    /// Data
    DockManagerData& _data;
    
    /// Value Tree
    juce::ValueTree _tree;
    
    /// Components
    juce::ShapeButton _closeButton;
    
    /// Utility
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabComponent)

};





/**
 -------------------------------------------------------------
 ===================================
 MARK: - Header Component -
 ===================================
 -------------------------------------------------------------
 */

class HeaderComponent : public juce::Component, private juce::ValueTree::Listener, public juce::DragAndDropTarget, public juce::DragAndDropContainer
{
public:
    HeaderComponent(DockManager& manager, DockManagerData& data, const juce::ValueTree& tree);
    ~HeaderComponent();

private:
    
    /// Component Override
    void paint(juce::Graphics &g) override;
    void resized() override;
    void animatedResize();
    
    /// Setup
    void setupTabs();
        
    /**
     ===================================
     MARK: - Getters -
     ===================================
     */
    
    const bool isTabs() const;
    const bool hasSubItems() const;
    const bool shouldShowTabs() const;
    const bool shouldShowHeader() const;
    const juce::String getUuid() const;
    const juce::String getViewName() const;
    const juce::String getDisplayName() const;
    
    const int getTabButtonWidth() const;
    const int getTabIndex(const juce::Point<int>& atPoint) const;
    const int getTabX(int atIndex) const;
    const int getNumVisibleTabs() const;

    /// Value Tree Listener
    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    void valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;
    
    /// Mouse
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void showViewRightClickMenu();
    void showTabRightClickMenu(juce::ValueTree tree);
    
    /// Drag and Drop Target
    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
    void itemDropped(const SourceDetails &dragSourceDetails) override;
    void itemDragEnter(const SourceDetails &dragSourceDetails) override;
    void itemDragExit(const SourceDetails &dragSourceDetails) override;
    void itemDragMove(const SourceDetails &dragSourceDetails) override;
    
    /// Drag and Drop Container
    void dragOperationStarted(const juce::DragAndDropTarget::SourceDetails& details) override;
    void dragOperationEnded(const juce::DragAndDropTarget::SourceDetails& details) override;
private:
    /// Dock Manager
    DockManager& _manager;
    
    /// Data
    DockManagerData& _data;
    
    /// Value Tree
    juce::ValueTree _tree;
    
    /// Tabs
    juce::OwnedArray<TabComponent> _tabs;
    std::unique_ptr<juce::Component> _tabHousing = nullptr;
    std::unique_ptr<juce::Viewport> _tabViewport = nullptr;
    
    /// Drag and Drop
    bool _isDragging = false;
    juce::Point<int> _draggingLocation;
    int _draggingIndex = -1;

    /// Sizes
    const int _minTabWidth = 75;
    const int _maxTabWidth = 120;
    
    /// Utility
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderComponent)

};


