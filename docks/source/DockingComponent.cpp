#include "DockingComponent.h"
#include "DockManager.h"
#include "DockManagerData.h"
#include "DockingWindow.h"

/**
 -------------------------------------------------------------
 ===================================
 MARK: - Resizer -
 ===================================
 -------------------------------------------------------------
 */

class ResizerBar : public juce::Component
{
public:
    ResizerBar(int index, bool vertical) : _index(index), _isVertical(vertical)
    {
        setMouseCursor(_isVertical ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::LeftRightResizeCursor);
        setName("Resize");
        setRepaintsOnMouseActivity(true);
    }
    
    std::function<void(juce::Point<float>, int)> onDrag = [](auto delta, auto index){};
    std::function<void()> finishedDrag = []{};
    
    const int getIndex() const {return _index;}
    const bool isVertical() const {return _isVertical;}
    
private:
    
    void paint(juce::Graphics &g) override
    {
        const auto& laf = getLookAndFeel().getDefaultLookAndFeel();
        auto resizeColor = laf.isColourSpecified(DockingWindow::ColourIds::backgroundColourId) ?    laf.findColour(DockingWindow::ColourIds::backgroundColourId) : juce::Colours::darkgrey;
        
        /// Color
        g.fillAll(isMouseOver() ? resizeColor.brighter() :  resizeColor);
        g.setColour(resizeColor.brighter());
        g.fillEllipse(getLocalBounds().toFloat().withSizeKeepingCentre(3, 3));
    }
    
    void mouseDown(const juce::MouseEvent &event) override
    {
        _initialPoint = event.position;
    }


    void mouseDrag(const juce::MouseEvent &event) override
    {
        if (!event.mouseWasDraggedSinceMouseDown()) {return;}
        auto delta = getMouseXYRelative().toFloat() - _initialPoint;
        onDrag(delta, _index);
        _initialPoint = getMouseXYRelative().toFloat();
    }


    void mouseUp(const juce::MouseEvent &event) override
    {
        if (event.mouseWasDraggedSinceMouseDown())
            finishedDrag();
        
        _initialPoint = {};
    }


private:
    int _index = -1;
    bool _isVertical = false;
    juce::Point<float> _initialPoint {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResizerBar)
};






/**
 -------------------------------------------------------------
 ===================================
 MARK: - Floater -
 ===================================
 -------------------------------------------------------------
 */
class Floater : public juce::Component
{
public:
    Floater(juce::Colour color = juce::Colours::blue.withAlpha(0.5f)) : _color(color) {}
    
    void paint(juce::Graphics &g) override
    {
        const auto& laf = getLookAndFeel().getDefaultLookAndFeel();
        auto color = laf.isColourSpecified(DockingComponent::ColourIds::draggingColorId) ?    laf.findColour(DockingComponent::ColourIds::draggingColorId) : _color;
        g.fillAll(color);
    }
    
private:
    juce::Colour _color;
};






/**
 -------------------------------------------------------------
 ===================================
 MARK: - Docking Component -
 ===================================
 -------------------------------------------------------------
 */

DockingComponent::DockingComponent(DockManager& manager, DockManagerData& data, const juce::ValueTree& tree) : _manager(manager), _data(data), _tree(tree)
{
    setSize(400, 400); /// Because it has to have some size...
    setupWithTree();
    auto name = _data.getName(tree);
    setName(name.isEmpty() ? "comp" : name);
    
    /// Floater
    _floater = std::make_unique<Floater>();
    addChildComponent(_floater.get());
    
    /// Setup View
    setupView();
    selectedTabDidChange();
}


DockingComponent::~DockingComponent()
{
    _tree.removeListener(this);
}





/**
 ===================================
 MARK: - Getters -
 ===================================
 */

const bool DockingComponent::hasSubItems() const
{
    return _tree.getNumChildren() > 0;
}


const bool DockingComponent::isTabs() const
{
    return _data.getDockType(_tree) == DockTypes::tabs;
}


const juce::String DockingComponent::getName() const
{
    return _data.getName(_tree);
}


const juce::String DockingComponent::getUuid() const
{
    return _data.getUuid(_tree);
}


const bool DockingComponent::shouldShowHeader() const
{
    return isTabs()
            || (!isTabs() && !hasSubItems() && _data.getDockType( _tree.getParent()) != DockTypes::tabs);
}


const juce::Rectangle<int> DockingComponent::getBoundsForSubview(const juce::String& uuid, int index) const
{
    if (uuid == getUuid())
    {
        if (index < _components.size() - 1 && index >= 0)
            return localAreaToGlobal(_components[index]->getBoundsInParent()); 
        else
            return localAreaToGlobal(getLocalBounds()).withTrimmedTop(shouldShowHeader() ? _headerHeight : 0);
            
    }
    for (auto component : _components)
    {
        auto bounds =  component->getBoundsForSubview(uuid, index);
        if (!bounds.isEmpty())
            return bounds;
    }
    
    return {};
}






/**
 ===================================
 MARK: - Component s -
 ===================================
 */

void DockingComponent::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds();
    const auto& laf = getLookAndFeel().getDefaultLookAndFeel();
    auto backgroundColor = laf.isColourSpecified(ColourIds::backgroundColourId) ? laf.findColour(ColourIds::backgroundColourId) : juce::Colours::lightgrey;
    
    /// Background Color
    g.fillAll(backgroundColor);
    
    /// Outline
    if (hasKeyboardFocus(true) && shouldShowHeader())
    {
        auto focusColor = laf.isColourSpecified(ColourIds::focusOutlineId) ? laf.findColour(ColourIds::focusOutlineId) : juce::Colours::blue;
        
        g.setColour(focusColor);
        g.drawRect(bounds, 1);
        g.drawRect(bounds.removeFromTop(_headerHeight).reduced(-1), 4);
    }
    
    /// Text
//    auto textColor = laf.isColourSpecified(ColourIds::textColorId) ? laf.findColour(ColourIds::textColorId) : juce::Colours::black;
//    g.setColour(textColor);
//    g.drawText(getName(), getLocalBounds(), juce::Justification::centred);
}


void DockingComponent::resized()
{
    auto bounds = getLocalBounds();
    if (_floater)
        _floater->setBounds(bounds);
    
    /// Header
    if (_header && shouldShowHeader())
        _header->setBounds(bounds.removeFromTop(_headerHeight).reduced(1));
    
    if (_view)
        _view->setBounds(bounds.reduced(1));
    
    switch (_data.getDockType(_tree))
    {
        case DockTypes::none:
        {
            for (auto component : _components)
                component->setBounds(bounds);
            break;
        }
        case DockTypes::tabs:
        {
            /// Components
            for (auto component : _components)
                component->setBounds(bounds.reduced(1));
            
            break;
        }
        case DockTypes::horizontal:
        {
            juce::Grid grid;
            using Track = juce::Grid::TrackInfo;
            using Px = juce::Grid::Px;
            using Fr = juce::Grid::Fr;
            grid.autoFlow = juce::Grid::AutoFlow::column;
            grid.templateRows.add(Track(Px(getHeight())));

            for (auto i = 0; i < _components.size(); i++)
            {
                auto child = _tree.getChild(i);
                if (child.isValid() && child.hasProperty(dockProps::widthProperty) && i != _components.size() - 2)
                {
                    grid.templateColumns.add(Track(Px(_data.getWidth(child))));
                }
                else
                {
                    grid.templateColumns.add(Track(Fr(1)));
                }
                
                grid.items.add(_components[i].get());
                
                if (i < _resizerBars.size())
                {
                    grid.templateColumns.add(Track(Px(_resizerSize)));
                    grid.items.add(_resizerBars[i].get());
                }
            }
            
            grid.performLayout(bounds);
            break;
        }
        case DockTypes::vertical:
        {
            juce::Grid grid;
            using Track = juce::Grid::TrackInfo;
            using Px = juce::Grid::Px;
            using Fr = juce::Grid::Fr;
            grid.autoFlow = juce::Grid::AutoFlow::row;
            grid.templateColumns.add(Track(Px(getWidth())));
            
            for (auto i = 0; i < _components.size(); i++)
            {
                auto child = _tree.getChild(i);
        
                if (child.isValid() && child.hasProperty(dockProps::heightProperty) && i != _components.size() - 2)
                {
                    grid.templateRows.add(Track(Px(_data.getHeight(child))));
                }
                else
                {
                    grid.templateRows.add(Track(Fr(1)));
                }
                grid.items.add(_components[i].get());
                
                if (i < _resizerBars.size())
                {
                    grid.templateRows.add(Track(Px(_resizerSize)));
                    grid.items.add(_resizerBars[i].get());
                }
            }
            
            grid.performLayout(bounds);
            break;
        }
    }


}


void DockingComponent::lookAndFeelChanged()
{
    repaint();

}





/**
 ===================================
 MARK: - Preset -
 ===================================
 */

void DockingComponent::layoutDidLoad()
{
    /// No Op atm
}








/**
 ====================================
 MARK: - Focus -
 ====================================
 */

void DockingComponent::focusOfChildComponentChanged(FocusChangeType cause)
{
    if (!isTabs() || !hasKeyboardFocus(true)) {repaint(); return;}
    
    auto selectedTab = _data.getSelectedId(_tree);
    for (auto tab : _components)
    {
        if (selectedTab == tab->getUuid())
        {
            tab->grabKeyboardFocus();
        }
    }
    repaint();
}





/**
 ===================================
 MARK: - Setup -
 ===================================
 */

void DockingComponent::setupWithTree()
{
    /// Add Listener
    _tree.addListener(this);
    
    /// Set The Name of the component
    auto name = _data.getName(_tree);
    setName(name);
    
    /// Check Tabs
    setupHeader();
    
    /// Add Subviews
    for (auto child : _tree)
        valueTreeChildAdded(_tree, child);
}


void DockingComponent::setupView()
{
    auto uuid = _data.getUuid(_tree);
    auto name = _data.getName(_tree);
    if (name.isEmpty() && _view)
    {
        removeChildComponent(_view.get());
     }
    
    /// Get the Component from the manager
    _view = _manager.getComponent(uuid, name);
    
    if (_view)
    {
        _view->setName(name); 
        if (!_view->getParentComponent())
            addAndMakeVisible(_view.get());
    }
    
    setupKeyboardFocus();
}


void DockingComponent::setupHeader()
{
    if (!shouldShowHeader())
    {
        _header = nullptr;
    }
    else
    {
        if (!_header)
        {
            _header = std::make_unique<HeaderComponent>(_manager, _data, _tree);
            addAndMakeVisible(_header.get());
            resized();
        }
    }
}


void DockingComponent::setupResizerBars()
{
    auto numExpected = _components.isEmpty() ? 0 : _components.size() - 1;
    auto isVertical = _data.getDockType(_tree) == DockTypes::vertical;
    if (_resizerBars.size() < numExpected)
    {
        for (auto i = _resizerBars.size(); i < numExpected; i++)
        {
            auto bar = std::make_shared<ResizerBar>(i, isVertical);
            _resizerBars.add(bar);
            bar->onDrag = [this](auto p, auto i) {resizerDidDrag(p, i);};
            bar->finishedDrag = [this] {resizerMouseUp();};
            
            addAndMakeVisible(bar.get());
        }
    }
    else if (numExpected < _resizerBars.size())
    {
        for (auto i = numExpected; i < _resizerBars.size(); i++)
            _resizerBars.removeLast();
    }
    
    resized();
}


void DockingComponent::setupKeyboardFocus()
{
    bool wantsFocus = isTabs() || _components.size() == 0;

    setWantsKeyboardFocus(wantsFocus);
    setMouseClickGrabsKeyboardFocus(wantsFocus);
}




/**
 ====================================
 MARK: - Selected Tab Did Change -
 ====================================
 */

void DockingComponent::selectedTabDidChange()
{
    if (!isTabs()) {return;}
    auto selectedTab = _data.getSelectedId(_tree);
    for (auto tab : _components)
        tab->setVisible(selectedTab == tab->getUuid());
    
    /// Reset Focus
    focusOfChildComponentChanged(FocusChangeType::focusChangedDirectly);
}





/**
 ===================================
 MARK: - Value Tree Listener -
 ===================================
 */

void DockingComponent::valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded)
{
    if (parentTree != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Added: Component " << _data.getUuid(childWhichHasBeenAdded));
    
    /// Create Subview
    auto id = _data.getUuid(childWhichHasBeenAdded);
    auto newView = std::make_shared<DockingComponent>(_manager, _data, childWhichHasBeenAdded);
    auto index = parentTree.indexOf(childWhichHasBeenAdded);
    _components.insert(index, newView);
    
    if (!isTabs())
        addAndMakeVisible(newView.get());
    else
        addChildComponent(newView.get());
    
    /// Select a tab if none selected
    if (_data.getSelectedId(_tree).isEmpty() && _data.getDockType(_tree) == DockTypes::tabs && _tree.getNumChildren() > 0)
    {
        _data.setSelected(_tree, _data.getUuid(_tree.getChild(0)));
    }
    
    /// Setup
    selectedTabDidChange();
    setupResizerBars();
    setupHeader();
    setupKeyboardFocus();
    resizeParent();
    repaint();
}


void DockingComponent::valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    if (parentTree != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Removed: Component " << _data.getUuid(childWhichHasBeenRemoved));
    
    /// Get Id
    _components.remove(indexFromWhichChildWasRemoved);
    setupHeader();
    setupResizerBars();
    setupKeyboardFocus();
}


void DockingComponent::valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged)
{
    if (treeWhoseParentHasChanged != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Parent Changed: Component " << _data.getUuid(treeWhoseParentHasChanged));
    setupHeader();
    setupKeyboardFocus();
}


void DockingComponent::valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex)
{
    if (parentTreeWhoseChildrenHaveMoved != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Order Changed: Component " << _data.getUuid(parentTreeWhoseChildrenHaveMoved));
    setupHeader();
    setupKeyboardFocus();
}


void DockingComponent::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    if (treeWhosePropertyHasChanged != _tree) {return;}
    
    if (property.toString() == dockProps::xProperty || property.toString() == dockProps::yProperty)
    {
        resizeParent();
    }
    else if (property.toString() == dockProps::widthProperty || property.toString() == dockProps::heightProperty)
    {
        resizeParent();
    }
    else if (property.toString() == dockProps::selectedProperty)
    {
        selectedTabDidChange();
    }
    else if (property.toString() == dockProps::dockType)
    {
        resized();
    }
    else if (property.toString() == dockProps::nameProperty)
    {
        setupView();
    }
    
    setupKeyboardFocus();
    setupHeader();
    repaint();
}





/**
 ===================================
 MARK: - Drag and Drop Target -
 ===================================
 */

bool DockingComponent::isInterestedInDragSource(const SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.sourceComponent.get())
        return dynamic_cast<TabComponent*>(dragSourceDetails.sourceComponent.get()) != nullptr
            || dynamic_cast<HeaderComponent*>(dragSourceDetails.sourceComponent.get()) != nullptr;
    return false;
}


void DockingComponent::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    itemDragMove(dragSourceDetails);
}


void DockingComponent::itemDragExit(const SourceDetails &dragSourceDetails)
{
    auto rootComponent = findParentComponentOfClass<WindowComponent>();
    if (!rootComponent) {repaint(); return;}
    rootComponent->hideDropHandle();
}


void DockingComponent::itemDragMove(const SourceDetails &dragSourceDetails)
{
    auto location = getDragLocation(getMouseXYRelative());
    auto rootComponent = findParentComponentOfClass<WindowComponent>();
    if (!rootComponent) {repaint(); return;}
    auto treeThatIsMoving = dragSourceDetails.description[0].toString();
    auto [treeToDropAt, index] = _data.getTreeForDockLocation(getUuid(), location);
    
    if (_data.isRootTree(treeToDropAt))
    {
        switch (location)
        {
            case DropLocation::parentLeft:
            case DropLocation::viewLeft:
            {
                location = DropLocation::rootLeft;
                break;
            }
            case DropLocation::parentRight:
            case DropLocation::viewRight:
            {
                location = DropLocation::rootRight;
                break;
            }
            case DropLocation::parentTop:
            case DropLocation::viewTop:
            {
                location = DropLocation::rootTop;
                break;
            }
            case DropLocation::parentBottom:
            case DropLocation::viewBottom:
            {
                location = DropLocation::rootBottom;
                break;
            }
            default:
                break;
        }
    }
//
//    DBG("Tree to Drop At: " << _data.getName(treeToDropAt)
//        <<", Dragging: " << _data.getName(treeThatIsMoving)
//        << ", Location: " << _data.dropLocationToString(location)
//        << ", Index: " << index);
//
    if (treeToDropAt == treeThatIsMoving || getUuid() == treeThatIsMoving)
        rootComponent->hideDropHandle();
    else
        rootComponent->showDropHandleAt(treeToDropAt, index, location);
}


void DockingComponent::itemDropped(const SourceDetails &dragSourceDetails)
{
    /// Tell Manager Not To create new view
    _manager.setCreateNewView(false);
    
    /// Hide Root Drop Component
    if (auto rootComponent = findParentComponentOfClass<WindowComponent>())
        rootComponent->hideDropHandle();
    
    /// Get Data
    auto location = getDragLocation(getMouseXYRelative());
    auto [treeToDropAt, index] = _data.getTreeForDockLocation(getUuid(), location);
    auto viewToDock = dragSourceDetails.description[0].toString();

    DBG("Tree to Drop At: " << _data.getName(treeToDropAt)
        <<", Dragging: " << _data.getName(viewToDock)
        << ", Location: " << _data.dropLocationToString(location)
        << ", Point: " << localPointToGlobal(getMouseXYRelative()).toString());
    
    /// Drop
    _data.dockView(viewToDock, treeToDropAt, location, localPointToGlobal(getMouseXYRelative()).toFloat());
}


const DropLocation DockingComponent::getDragLocation(const juce::Point<int> position) const
{
    auto bounds = getLocalBounds();
    if (shouldShowHeader())
        bounds.removeFromTop(_headerHeight);
    
    for (auto resize : _resizerBars)
        if (resize->getBoundsInParent().contains(position))
            return DropLocation::none;
    
    if (auto peer = getPeer())
    {
        auto peerPosition = localPointToGlobal(position);
        auto peerBounds = peer->getBounds().withPosition(0, 0).reduced(5);
        auto rootTop = peerBounds.removeFromTop(_rootHitSize);
        auto rootBottom = peerBounds.removeFromBottom(_rootHitSize);
        auto rootLeft = peerBounds.removeFromLeft(_rootHitSize);
        auto rootRight = peerBounds.removeFromRight(_rootHitSize);
        
        if (rootTop.contains(peerPosition))
            return DropLocation::rootTop;
        else if (rootBottom.contains(peerPosition))
            return DropLocation::rootBottom;
        else if (rootLeft.contains(peerPosition))
            return DropLocation::rootLeft;
        else if (rootRight.contains(peerPosition))
            return DropLocation::rootRight;
    }
    
    auto parentTop = bounds.removeFromTop(_parentHitSize);
    auto parentBottom = bounds.removeFromBottom(_parentHitSize);
    auto parentLeft = bounds.removeFromLeft(_parentHitSize);
    auto parentRight = bounds.removeFromRight(_parentHitSize);
    auto viewTop = bounds.removeFromTop(_viewHitSize);
    auto viewBottom = bounds.removeFromBottom(_viewHitSize);
    auto viewLeft = bounds.removeFromLeft(_viewHitSize);
    auto viewRight = bounds.removeFromRight(_viewHitSize);
    
    if (parentTop.contains(position))
        return DropLocation::parentTop;
    else if (parentBottom.contains(position))
        return DropLocation::parentBottom;
    else if (parentLeft.contains(position))
        return DropLocation::parentLeft;
    else if (parentRight.contains(position))
        return DropLocation::parentRight;
    else if (viewTop.contains(position))
        return DropLocation::viewTop;
    else if (viewBottom.contains(position))
        return DropLocation::viewBottom;
    else if (viewLeft.contains(position))
        return DropLocation::viewLeft;
    else if (viewRight.contains(position))
        return DropLocation::viewRight;
    else if (bounds.contains(position))
        return DropLocation::tabs;
    else
        return DropLocation::none;
}





/**
 ==================================
 MARK: - Drag and Drop Container -
 ===================================
 */

void DockingComponent::dragOperationStarted(const juce::DragAndDropTarget::SourceDetails& details)
{
    if (details.sourceComponent == nullptr) {return;}
    if (auto comp = dynamic_cast<TabComponent*>(details.sourceComponent.get()))
        comp->setVisible(false);
    
    else if (auto comp = dynamic_cast<HeaderComponent*>(details.sourceComponent.get()))
        comp->setVisible(false);
    
    _manager.setCreateNewView(true);
}


void DockingComponent::dragOperationEnded(const juce::DragAndDropTarget::SourceDetails& details)
{
    /// Check for the correct type
    if (!details.description.isArray()) {return;}

    /// Show the source component 
    if (auto comp = details.sourceComponent)
        comp->setVisible(true);
    
    /// Get Dragging Id
    auto draggingId = details.description[0].toString();
    
    /// Create new Window (Manager take care of if Applicable);
    _manager.createNewWindow(draggingId, localPointToGlobal(getMouseXYRelative()).toFloat());
}





/**
 ===================================
 MARK: - Resizer -
 ===================================
 */

void DockingComponent::resizerDidDrag(juce::Point<float> delta, int index)
{
    if (index >= _components.size()) {return;}
 
    /// Chr
    auto tree = _tree.getChild(index);
    auto comp = _components[index];
    
    /// Next
    auto nextTree = _tree.getChild(index + 1);
    auto nextComp = index + 1 > _components.size() - 1 ? nullptr : _components[index + 1];
    
    auto resize = dynamic_cast<ResizerBar*>(_resizerBars[index].get());
    if (!tree.isValid() || comp == nullptr || resize == nullptr) {return;}
    
    auto size = _data.getSize(tree);
    auto nextSize = nextTree.isValid() ? _data.getSize(nextTree) : juce::Point<float>();
    auto isOrigin = size.isOrigin();
    auto vertical = resize->isVertical();
    
    if (vertical)
    {
        _data.setHeight(tree, isOrigin ? comp->getHeight() + delta.y : size.y + delta.y);
        if (nextTree.isValid() && nextComp != nullptr)
            _data.setHeight(nextTree, nextSize.isOrigin() ? nextComp->getHeight() - delta.y : nextSize.y - delta.y);
    }
    else
    {
        _data.setWidth(tree, isOrigin ? comp->getWidth() + delta.x : size.x + delta.x);
        if (nextTree.isValid() && nextComp != nullptr)
            _data.setWidth(nextTree, nextSize.isOrigin() ? nextComp->getWidth() - delta.x : nextSize.x - delta.x);
    }
    
    resized();
    checkWillDisappear();
}


void DockingComponent::resizerMouseUp()
{
    checkWillDisappear();
    checkViewsShouldExist();
}


void DockingComponent::checkWillDisappear()
{
    _willDisappear = getWidth() < _minimumSize || getHeight() < _minimumSize;
    for (auto comp : _components)
        comp->checkWillDisappear();
    
    if (_floater)
    {
        _floater->setVisible(_willDisappear);
        _floater->toFront(false);
    }
}


void DockingComponent::checkViewsShouldExist()
{
    auto shouldNotExist = getWidth() < _minimumSize || getHeight() < _minimumSize;
    if (shouldNotExist)
    {
        _data.removeViewAndChildren(getUuid());
    }
    else
    {
        for (auto comp : _components)
            if (comp)
                comp->checkViewsShouldExist();
    }
}


void DockingComponent::resizeParent()
{
    if (auto parent = getParentComponent())
        parent->resized();
}


void DockingComponent::resetDisplayName()
{
    repaint();
}



/**
 ====================================
 MARK: - Keyboard -
 ====================================
 */

bool DockingComponent::keyPressed(const juce::KeyPress& key)
{
    if (!hasKeyboardFocus(true) || !key.isCurrentlyDown()) {return false;}
    if (keyPressed_tabs(key))
        return true;
    else if (changeFocusTo(key))
        return true;

    return false;
}


bool DockingComponent::keyPressed_tabs(const juce::KeyPress& key)
{
    if (!isTabs()) {return false;}
    auto tabMod = juce::ModifierKeys::altModifier;
    if (key == juce::KeyPress('1', tabMod, 0))
        return selectTab(0);
    else if (key == juce::KeyPress('2', tabMod, 0))
        return selectTab(1);
    else if (key == juce::KeyPress('3', tabMod, 0))
        return selectTab(2);
    else if (key == juce::KeyPress('4', tabMod, 0))
        return selectTab(3);
    else if (key == juce::KeyPress('5', tabMod, 0))
        return selectTab(4);
    else if (key == juce::KeyPress('6', tabMod, 0))
        return selectTab(5);
    else if (key == juce::KeyPress('7', tabMod, 0))
        return selectTab(6);
    else if (key == juce::KeyPress('8', tabMod, 0))
        return selectTab(7);
    else if (key == juce::KeyPress('9', tabMod, 0))
        return selectTab(8);
    else if (key == juce::KeyPress('0', tabMod, 0))
        return selectTab(9);
    else if (key == juce::KeyPress(juce::KeyPress::tabKey, tabMod, 0))
        return selectNextTab();
    else if (key == juce::KeyPress(juce::KeyPress::tabKey, tabMod | juce::ModifierKeys::shiftModifier, 0))
        return selectPreviousTab();
    
    return false;
}


bool DockingComponent::selectNextTab()
{
    auto selected = _data.getSelectedId(_tree);
    auto child = _tree.getChildWithProperty(dockProps::uuidProperty, selected);
    if (!child.isValid()) {return false;}
    auto index = _tree.indexOf(child) + 1;
    if (index >= _tree.getNumChildren())
        index = 0;
    return selectTab(index);
}


bool DockingComponent::selectPreviousTab()
{
    auto selected = _data.getSelectedId(_tree);
    auto child = _tree.getChildWithProperty(dockProps::uuidProperty, selected);
    if (!child.isValid()) {return false;}
    auto index = _tree.indexOf(child) - 1;
    if (index < 0)
        index = _tree.getNumChildren() - 1;
    return selectTab(index);
}


bool DockingComponent::selectTab(int index)
{
    auto child = _tree.getChild(index);
    if (!child.isValid()) {return false;}
    _data.setSelected(_tree, _data.getUuid(child));
    return true;
}


bool DockingComponent::changeFocusTo(const juce::KeyPress& key)
{
    auto component = getTopLevelComponent();
    if (!component) {return false;}
    juce::Point<int> point;
    auto globalBounds = localAreaToGlobal(getLocalBounds());
    auto mod = juce::ModifierKeys::altModifier;
    if (key == juce::KeyPress(juce::KeyPress::upKey, mod, 0))
        point = juce::Point<int>(globalBounds.getCentreX(), globalBounds.getY() - _headerHeight);
    else if (key == juce::KeyPress(juce::KeyPress::downKey, mod, 0))
        point = juce::Point<int>(globalBounds.getCentreX(), globalBounds.getBottom() + _headerHeight);
    else if (key == juce::KeyPress(juce::KeyPress::leftKey, mod, 0))
        point = juce::Point<int>(globalBounds.getX() - _headerHeight, globalBounds.getCentreY());
    else if (key == juce::KeyPress(juce::KeyPress::rightKey, mod, 0))
        point = juce::Point<int>(globalBounds.getRight() + _headerHeight, globalBounds.getCentreY());
    else
        return false;
    
    auto child = component->getComponentAt(component->getLocalPoint(nullptr, point));
    if (!child) {return false;}
    child->grabKeyboardFocus();
    return true;
}
