#include "HeaderComponent.h"
#include "DockManager.h"
#include "DockManagerData.h"
#include "DockingComponent.h"



TabComponent::TabComponent(DockManager& manager, DockManagerData& data, const juce::ValueTree& tree) : _manager(manager), _data(data), _tree(tree), _closeButton("Close", juce::Colours::darkgrey.brighter(), juce::Colours::white, juce::Colours::blue)
{
    
    _tree.addListener(this);
    
    /// Close Button
    juce::Path p;
    const int height = 20;
    p.addLineSegment(juce::Line<float>(0, 0, height, height), 3);
    p.addLineSegment(juce::Line<float>(0, height, height, 0), 3);
    _closeButton.setShape(p, true, true, false);
    
    addAndMakeVisible(_closeButton);
    _closeButton.onClick = [this]{_manager.removeView(getUuid());};
    
    /// Remove a focus outline (if it has one)
    setHasFocusOutline(false);
}

TabComponent::~TabComponent()
{
    _tree.removeListener(this);
}





/**
 ===================================
 MARK: - Getters -
 ===================================
 */

const juce::String TabComponent::getUuid() const
{
    return _data.getUuid(_tree);
}


const juce::ValueTree TabComponent::getTree() const
{
    return _tree;
}


const bool TabComponent::getSelected() const
{
    if (!_tree.getParent().isValid()) {return false;}
    return _data.getSelectedId(_tree.getParent()) == _data.getUuid(_tree);
}


const juce::String TabComponent::getDisplayName() const
{
    return _manager._delegate.getDisplayNameForView(_data.getName(_tree));
}



/**
 ===================================
 MARK: - Component Overrides -
 ===================================
 */

void TabComponent::resized()
{
    auto bounds = getLocalBounds();
    const int size = 8;
    _closeButton.setBounds(bounds.removeFromRight(getHeight()).withSizeKeepingCentre(size, size));
}


void TabComponent::paint(juce::Graphics &g)
{
    const auto& laf = getLookAndFeel().getDefaultLookAndFeel();
    auto tabBackgroundColor = laf.isColourSpecified(DockingComponent::ColourIds::tabColorId) ?    laf.findColour(DockingComponent::ColourIds::tabColorId) : juce::Colours::darkgrey;
    auto tabSelectedColor = laf.isColourSpecified(DockingComponent::ColourIds::tabSelectedColorId) ?    laf.findColour(DockingComponent::ColourIds::tabSelectedColorId) : juce::Colours::blue.withAlpha(0.5f);
    auto tabBorderColor = laf.isColourSpecified(DockingComponent::ColourIds::tabBorderColorId) ?    laf.findColour(DockingComponent::ColourIds::tabBorderColorId) : juce::Colours::darkgrey.brighter();
    auto textColor = laf.isColourSpecified(DockingComponent::ColourIds::textColorId) ?    laf.findColour(DockingComponent::ColourIds::textColorId) : juce::Colours::white;
    
    auto selected = getSelected();
    g.fillAll(selected ? tabSelectedColor : tabBackgroundColor);
    
    /// Border
    g.setColour(tabBorderColor);
    g.drawLine(0, 0, 0, getHeight());
    g.drawLine(getWidth(), 0, getWidth(), getHeight());

    /// Text
    auto bounds = getLocalBounds().withTrimmedRight(getHeight());
    auto name = getDisplayName();
    g.setColour(textColor);
    g.drawText(name, bounds, juce::Justification::centred);
}





/**
 ===================================
 MARK: - Value Tree Listener -
 ===================================
 */

void TabComponent::valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded)
{
    if (parentTree != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Added: Tab " << _data.getUuid(childWhichHasBeenAdded));
}


void TabComponent::valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    if (parentTree != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Removed: Tab " << _data.getUuid(childWhichHasBeenRemoved));
}


void TabComponent::valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged)
{
    if (treeWhoseParentHasChanged != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Parent Changed: Tab " << _data.getUuid(treeWhoseParentHasChanged));
}


void TabComponent::valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex)
{
    if (parentTreeWhoseChildrenHaveMoved != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Order Changed: Tab " << _data.getUuid(parentTreeWhoseChildrenHaveMoved));
}


void TabComponent::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    if (treeWhosePropertyHasChanged != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Property Changed: Tab " << _data.getUuid(treeWhosePropertyHasChanged) << " " << property.toString());
}





/**
 -------------------------------------------------------------
 ===================================
 MARK: - Header Component -
 ===================================
 -------------------------------------------------------------
 */

HeaderComponent::HeaderComponent(DockManager& manager, DockManagerData& data, const juce::ValueTree& tree) : _manager(manager), _data(data), _tree(tree)
{
    _tree.addListener(this);
    _tabViewport = std::make_unique<juce::Viewport>();
    _tabHousing = std::make_unique<juce::Component>();
    _tabViewport->setViewedComponent(_tabHousing.get());
    _tabViewport->setScrollBarsShown(false, false, false, true);
    addChildComponent(_tabViewport.get());
    setupTabs();
    setHasFocusOutline(false);
}


HeaderComponent::~HeaderComponent()
{
    _tabs.clear();
    _tabHousing = nullptr;
    if(_tabViewport)
        _tabViewport->setViewedComponent(nullptr);
    _tabViewport = nullptr;
    _tree.removeListener(this);
}





/**
 ===================================
 MARK: - Getters -
 ===================================
 */

const bool HeaderComponent::isTabs() const
{
    return _data.getDockType(_tree) == DockTypes::tabs;
}


const bool HeaderComponent::hasSubItems() const
{
    return _tree.getNumChildren() > 0;
}


const bool HeaderComponent::shouldShowTabs() const
{
    return isTabs();
}


const bool HeaderComponent::shouldShowHeader() const
{
    return (isTabs() && !hasSubItems()) || (!isTabs() && !hasSubItems());
}


const juce::String HeaderComponent::getUuid() const
{
    return _data.getUuid(_tree);
}


const juce::String HeaderComponent::getViewName() const
{
    return _data.getName(_tree);
}


const juce::String HeaderComponent::getDisplayName() const
{
    return _manager._delegate.getDisplayNameForView(getViewName());
}


const int HeaderComponent::getTabButtonWidth() const
{
    if (_tabs.size() == 0)
        return _maxTabWidth;
    
    auto parentWidth = getParentWidth();
    return juce::jmax(juce::jmin(parentWidth / _tabs.size(), _maxTabWidth), _minTabWidth);

}


const int HeaderComponent::getTabIndex(const juce::Point<int>& atPoint) const
{
    if (_tabs.isEmpty()) {return 0;}
    return atPoint.x / getTabButtonWidth();
}


const int HeaderComponent::getTabX(int atIndex) const
{
    return atIndex * getTabButtonWidth();
}


const int HeaderComponent::getNumVisibleTabs() const
{
    int i = 0;
    for (auto tab : _tabs)
        if (tab->isVisible())
            i++;
    return i;
}





/**
 ===================================
 MARK: - Component Overrides -
 ===================================
 */

void HeaderComponent::paint(juce::Graphics &g)
{
    const auto& laf = getLookAndFeel().getDefaultLookAndFeel();
    auto headerBackground = laf.isColourSpecified(DockingComponent::ColourIds::headerBackgroundColorId) ?    laf.findColour(DockingComponent::ColourIds::headerBackgroundColorId) : juce::Colours::darkgrey;
    
    /// Background
    g.fillAll(headerBackground);

    /// Name
    if (!isTabs())
    {
        auto textColor = laf.isColourSpecified(DockingComponent::ColourIds::textColorId) ?    laf.findColour(DockingComponent::ColourIds::textColorId) : juce::Colours::white;

        auto name = getDisplayName();
        g.setColour(textColor);
        g.drawText(name, getLocalBounds(), juce::Justification::centred);
    }
    
    
    if (_isDragging)
    {
        auto draggingColor = laf.isColourSpecified(DockingComponent::ColourIds::draggingColorId) ?    laf.findColour(DockingComponent::ColourIds::draggingColorId) : juce::Colours::white;

        if (!isTabs())
        {
            g.fillAll(draggingColor.withAlpha(0.5f));
            g.setColour(draggingColor);
            g.drawRect(getBounds(), 2);
        }
        else
        {
            auto visibleTabs = getNumVisibleTabs();
            auto index = getTabIndex(_draggingLocation);
            auto dragX = getTabX(std::clamp<int>(index, 0, visibleTabs));
            if (index >= visibleTabs)
            {
                auto rect = juce::Rectangle<int>(dragX, 0, getTabButtonWidth(), getHeight());
                g.setColour(draggingColor.withAlpha(0.5f));
                g.fillRect(rect);
                g.setColour(draggingColor);
                g.drawRect(rect);
            }
            else
            {
                g.setColour(draggingColor);
                g.fillRect(dragX, 0, 7, getHeight());
            }
        }
    }
}


void HeaderComponent::resized()
{
    if (_tabs.size() == 0) {return;}
    auto width = getTabButtonWidth();

    if (_tabViewport)
        _tabViewport->setBounds(getLocalBounds().reduced(2, 1));
    
    if (_tabHousing)
        _tabHousing->setSize(width * _tabs.size() + 10, getHeight());

    juce::Rectangle<int> bounds = {0, 0, width * _tabs.size(), getHeight()};
    for (auto tab : _tabs)
    {
        if (!tab->isVisible()) {continue;}
        auto tabBounds = bounds.removeFromLeft(width);
        tab->setBounds(tabBounds);
    }
}


void HeaderComponent::animatedResize()
{
    if (_tabs.size() == 0) {return;}
    
    auto width = getTabButtonWidth();
    juce::Rectangle<int> bounds = {0, 0, width * _tabs.size(), getHeight()};
    auto& animator = juce::Desktop::getInstance().getAnimator();
    int index = 0;
    for (auto i = 0; i < _tabs.size(); i++)
    {
        auto tab = _tabs[i];
        if (!tab->isVisible()) {continue;}
        if (_draggingIndex == index)
            bounds.removeFromLeft(7);
        auto tabBounds = bounds.removeFromLeft(width);
        animator.animateComponent(tab, tabBounds, 1.0, 100, false, 1.0, 0.5);
        index++;
    }

}



/**
 ===================================
 MARK: - Setup Tabs -
 ===================================
 */

void HeaderComponent::setupTabs()
{
    bool showTabs = shouldShowTabs();
    if (_tabViewport)
        _tabViewport->setVisible(showTabs);
    
    if (!showTabs) {repaint(); return;}

    _tabs.clear();
    auto selected = _data.getSelectedId(_tree);
    
    for (auto child : _tree)
    {
        auto tab = _tabs.add(new TabComponent(_manager, _data, child));
        tab->addMouseListener(this, true);
        _tabHousing->addAndMakeVisible(tab);
    }
    
    resized();
}





/**
 ===================================
 MARK: - Value Tree Listener -
 ===================================
 */

void HeaderComponent::valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded)
{
    if (parentTree != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Added: Header " << _data.getUuid(childWhichHasBeenAdded));
    setupTabs();
}


void HeaderComponent::valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    if (parentTree != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Removed: Header " << _data.getUuid(childWhichHasBeenRemoved));
    setupTabs();
}


void HeaderComponent::valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged)
{
    if (treeWhoseParentHasChanged != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Parent Changed: Header " << _data.getUuid(treeWhoseParentHasChanged));
    
    setupTabs();
}


void HeaderComponent::valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex)
{
    if (parentTreeWhoseChildrenHaveMoved != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Order Changed: Header " << _data.getUuid(parentTreeWhoseChildrenHaveMoved));
}


void HeaderComponent::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    if (treeWhosePropertyHasChanged != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Property Changed: Header " << _data.getUuid(treeWhosePropertyHasChanged) << " " << property.toString());
    if (property.toString() == dockProps::selectedProperty)
        repaint();
}





/**
 ===================================
 MARK: - Mouse -
 ===================================
 */

void HeaderComponent::mouseDown(const juce::MouseEvent& event)
{
    if (auto comp = dynamic_cast<TabComponent*>(event.eventComponent))
    {
        _data.setSelected(_tree, comp->getUuid());
        if (event.mods.isRightButtonDown())
            showTabRightClickMenu(comp->getTree());
    }
    else
    {
        if (event.mods.isRightButtonDown())
            showViewRightClickMenu();
    }
}


void HeaderComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (event.eventComponent == this)
    {
        /// Don't Drag if point is in bounds
        if (getLocalBounds().contains(event.position.toInt())) {return;}
        
        /// Begin Drag and Drop Session
        juce::var data;
        data.append(_data.getUuid(_tree));
        if (auto container = juce::DragAndDropContainer::findParentDragContainerFor(this))
            container->startDragging(data, this, juce::ScaledImage(), true);
    }
    else
    {
        auto comp = dynamic_cast<TabComponent*>(event.eventComponent);
        if (!comp) {return;}
        
        /// Don't Drag if point is in bounds
        if (comp->getLocalBounds().contains(event.position.toInt())) {return;}
        
        /// Begin Drag and Drop Session
        juce::var data;
        data.append(comp->getUuid());
        if (auto container = juce::DragAndDropContainer::findParentDragContainerFor(this))
            container->startDragging(data, comp, juce::ScaledImage(), true);
    }
}


void HeaderComponent::mouseUp(const juce::MouseEvent& event)
{

}


void HeaderComponent::showViewRightClickMenu()
{
    juce::PopupMenu menu = _manager.getHeaderPopupMenu(_tree);
    menu.showMenuAsync(juce::PopupMenu::Options());
}


void HeaderComponent::showTabRightClickMenu(juce::ValueTree tree)
{
    auto menu = _manager.getTabPopupMenu(tree);
    menu.showMenuAsync(juce::PopupMenu::Options());
}





/**
 ===================================
 MARK: - Drag and Drop Target -
 ===================================
 */

bool HeaderComponent::isInterestedInDragSource(const SourceDetails &dragSourceDetails)
{
    if (dynamic_cast<TabComponent*>(dragSourceDetails.sourceComponent.get()) != nullptr)
        return true;
    else if (dynamic_cast<HeaderComponent*>(dragSourceDetails.sourceComponent.get()) != nullptr)
        return true;
    else
        return false;
}


void HeaderComponent::itemDropped(const SourceDetails &dragSourceDetails)
{
    _isDragging = false;
    _manager.setCreateNewView(false);
    
    juce::String uuid = dragSourceDetails.description[0];
    _data.dockView(uuid, getUuid(), DropLocation::tabs, {}, _draggingIndex);
    repaint();
}


void HeaderComponent::itemDragEnter(const SourceDetails &dragSourceDetails)
{
    _isDragging = true;
    repaint();
}


void HeaderComponent::itemDragExit(const SourceDetails &dragSourceDetails)
{
    _isDragging = false;
    repaint();
}


void HeaderComponent::itemDragMove(const SourceDetails &dragSourceDetails)
{
    _isDragging = true;
    _draggingLocation = getLocalPoint(nullptr, juce::Desktop::getInstance().getMousePosition());
    repaint();
    
    auto index = getTabIndex(_draggingLocation);
    if (index != _draggingIndex)
    {
        _draggingIndex = index;
        animatedResize();
    }
}



/**
 ==================================
 MARK: - Drag and Drop Container -
 ===================================
 */

void HeaderComponent::dragOperationStarted(const juce::DragAndDropTarget::SourceDetails& details)
{
    if (details.sourceComponent == nullptr) {return;}
    if (auto comp = dynamic_cast<TabComponent*>(details.sourceComponent.get()))
        comp->setVisible(false);
    else if (auto comp = dynamic_cast<HeaderComponent*>(details.sourceComponent.get()))
        comp->setVisible(false);
    
    /// Tell Manager to create view when this completes
    _manager.setCreateNewView(true);
    
    /// Animate the views
    animatedResize();
}


void HeaderComponent::dragOperationEnded(const juce::DragAndDropTarget::SourceDetails& details)
{
    _draggingIndex = -1;
    _isDragging = false;
    _draggingLocation = {};
    
    if (auto comp = details.sourceComponent)
        comp->setVisible(true);
    
    /// Resize back
    animatedResize();
    
    /// Get Dragging Id
    auto draggingId = details.description[0].toString();
    if (draggingId.isEmpty()) {return;}
    
    /// Create new Window (Manager take care of if Applicable);
    _manager.createNewWindow(draggingId, localPointToGlobal(getMouseXYRelative()).toFloat());
}
