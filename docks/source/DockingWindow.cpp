#include "DockingWindow.h"
#include "DockManager.h"
#include "DockManagerData.h"
#include "DockingComponent.h"
#include "BinaryData.h"

WindowDropHandle::WindowDropHandle()
{
    setInterceptsMouseClicks(false, false);
}


void WindowDropHandle::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    const int lineWidth = 5;
    const auto& laf = getLookAndFeel().getDefaultLookAndFeel();
    auto color = laf.isColourSpecified(DockingComponent::ColourIds::draggingColorId) ?    laf.findColour(DockingComponent::ColourIds::draggingColorId) : juce::Colours::blue;

    switch (_type)
    {
        case DropLocation::rootTop:
        case DropLocation::rootBottom:
        {
            g.setColour(color.withAlpha(0.5f));
            g.fillAll();
            JUCE_FALLTHROUGH;
        }
        case DropLocation::viewTop:
        case DropLocation::viewBottom:
        case DropLocation::parentTop:
        case DropLocation::parentBottom:
        {
            juce::Path p;
            bounds = bounds.reduced(lineWidth, 0);
            p.startNewSubPath(bounds.getX(), bounds.getCentreY());
            p.lineTo(bounds.getRight(), bounds.getCentreY());
            
            /// Draw Path
            g.setColour(color);
            g.strokePath(p, juce::PathStrokeType(lineWidth, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));
            
            break;
        }
        case DropLocation::rootLeft:
        case DropLocation::rootRight:
        {
            g.setColour(color.withAlpha(0.5f));
            g.fillAll();
            JUCE_FALLTHROUGH;
        }
        case DropLocation::viewLeft:
        case DropLocation::viewRight:
        case DropLocation::parentLeft:
        case DropLocation::parentRight:
        {
            juce::Path p;
            bounds = bounds.reduced(0, lineWidth);
            p.startNewSubPath(bounds.getCentreX(), bounds.getY());
            p.lineTo(bounds.getCentreX(), bounds.getBottom());
            
            /// Draw Path
            g.setColour(color);
            g.strokePath(p, juce::PathStrokeType(lineWidth, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));
            
            break;
        }
        case DropLocation::tabs:
        {
            juce::Path p;
            const int lineHeight = 50;
            bounds = bounds.reduced(10);
            
            /// Top Left
            p.startNewSubPath(bounds.getX(), lineHeight);
            p.lineTo(bounds.getX(), bounds.getY());
            p.lineTo(lineHeight, bounds.getY());
            
            /// Top Right
            p.startNewSubPath(bounds.getRight() - lineHeight, bounds.getY());
            p.lineTo(bounds.getRight(), bounds.getY());
            p.lineTo(bounds.getRight(), lineHeight);
            
            /// Bottom Right
            p.startNewSubPath(bounds.getRight(), bounds.getBottom() - lineHeight);
            p.lineTo(bounds.getRight(), bounds.getBottom());
            p.lineTo(bounds.getRight() - lineHeight, bounds.getBottom());
            
            /// Bottom Left;
            p.startNewSubPath(bounds.getX() + lineHeight, bounds.getBottom());
            p.lineTo(bounds.getX(), bounds.getBottom());
            p.lineTo(bounds.getX(), bounds.getBottom() - lineHeight);
            
            /// Middle Left
            p.startNewSubPath(bounds.getX(), bounds.getCentreY() - lineHeight/2);
            p.lineTo(bounds.getX(), bounds.getCentreY() + lineHeight/2);
            
            /// Middle Right
            p.startNewSubPath(bounds.getRight(), bounds.getCentreY() - lineHeight/2);
            p.lineTo(bounds.getRight(), bounds.getCentreY() + lineHeight/2);
            
            /// Fill
            g.setColour(color.withAlpha(0.3f));
            g.fillRect(bounds);
            
            /// Stroke Path
            g.setColour(color);
            auto strokeType = juce::PathStrokeType(lineWidth, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded);
            g.strokePath(p, strokeType);
            
            break;
        }
        case DropLocation::none:
        {
            break;
        }
    }
}


void WindowDropHandle::setDropLocation(DropLocation type)
{
    _type = type;
    repaint();
}





/**
 -------------------------------------------------------------
 ===================================
 MARK: - Window Component -
 ===================================
 -------------------------------------------------------------
 */


WindowComponent::WindowComponent(DockingWindow& window, DockManager& manager, DockManagerData& data, const juce::ValueTree& tree) : _window(window), _manager(manager), _data(data), _tree(tree)
{
    /// Listener
    _tree.addListener(this);
    
    /// Component Size/Name
    setSize(400, 400);
    setName("WindowComp");
    
    /// Drop Handle
    addChildComponent(_dropHandle);
    
    /// Setup Additional Views
    setupMenu();
    setupFooter();
    
    /// Refresh View
    refresh();
}


WindowComponent::~WindowComponent()
{
    _tree.removeListener(this);
}





/**
 ====================================
 MARK: - Component Overrides -
 ====================================
 */

void WindowComponent::resized()
{
    auto bounds = getLocalBounds();
    
    if (_overlay)
        _overlay->setBounds(getLocalBounds());
    
    if (_footerComponent != nullptr)
    {
        auto footerBounds = bounds.removeFromBottom(_footerComponent->getHeight() == 0 ? 18 : _footerComponent->getHeight());
        if (_lockedButton != nullptr)
            _lockedButton->setBounds(footerBounds.withTrimmedLeft(5).removeFromLeft(footerBounds.getHeight()).reduced(2));
        _footerComponent->setBounds(footerBounds);
    }
    else if (_lockedButton != nullptr)
    {
        const auto footerBounds = 18;
        _lockedButton->setBounds(bounds.removeFromBottom(footerBounds).withTrimmedLeft(5).removeFromLeft(footerBounds).reduced(2));
    }

#if (!JUCE_MAC)
    if (_menuComponent != nullptr && bounds.getWidth() > 300)
        _menuComponent->setBounds(bounds.removeFromTop(20));
#endif
    
    const int trim = 5;
    const int bottomTrim = _footerComponent != nullptr || _lockedButton != nullptr ? 0 : trim;
    if (_dockingComponent)
        _dockingComponent->setBounds(bounds.withTrimmedLeft(trim)
                                            .withTrimmedTop(trim)
                                            .withTrimmedRight(trim)
                                            .withTrimmedBottom(bottomTrim));
}


void WindowComponent::paint(juce::Graphics &g)
{
    const auto& laf = getLookAndFeel().getDefaultLookAndFeel();
    auto color = laf.isColourSpecified(DockingWindow::ColourIds::backgroundColourId) ?    laf.findColour(DockingComponent::ColourIds::backgroundColourId) : juce::Colours::darkgrey;
    g.fillAll(color);
    
    if (_data.isWindowLocked(_tree))
    {
        g.setColour(juce::Colours::lightyellow);
        g.drawRect(getLocalBounds());
    }
}






/**
 ====================================
 MARK: - Setup Views -
 ====================================
 */

void WindowComponent::setupMenu()
{
#if JUCE_MAC
    return;
#else
    auto name = _data.getName(_tree);
    if (name.isEmpty())
        name = _manager._delegate.getDefaultWindowName();

    _menuComponent = _manager._delegate.getMenuForWindow(name);
    if (_menuComponent == nullptr) {return;}
    addAndMakeVisible(_menuComponent.get());
#endif
}


void WindowComponent::setupFooter()
{
    auto name = _data.getName(_tree);
    if (name.isEmpty())
        name = _manager._delegate.getDefaultWindowName();
    
    _footerComponent = _manager._delegate.getFooterForWindow(name);
    if (_footerComponent == nullptr) {return;}
    addAndMakeVisible(_footerComponent.get());
}


void WindowComponent::layoutDidLoad()
{
    if (_dockingComponent)
        _dockingComponent->layoutDidLoad();
}


void WindowComponent::resetAllDisplayNames()
{
    if (_dockingComponent)
        _dockingComponent->resetDisplayName(); 
}


/**
 ====================================
 MARK: - Drop Handle  -
 ====================================
 */
void WindowComponent::showDropHandleAt(const juce::String& uuid, int index, DropLocation location)
{
    if (!_dockingComponent) {return;}
    auto bounds = getLocalArea(nullptr, _dockingComponent->getBoundsForSubview(uuid, index));
    _dropHandle.setDropLocation(location);
    _dropHandle.setVisible(true);
    _dropHandle.toFront(false);
    
    const int handleSize = 10;
    switch (location)
    {
        case DropLocation::tabs:
        {
            _dropHandle.setBounds(bounds);
            break;
        }
        case DropLocation::parentTop:
        case DropLocation::viewTop:
        {
            _dropHandle.setBounds(bounds.removeFromTop(handleSize));
            break;
        }
        case DropLocation::parentBottom:
        case DropLocation::viewBottom:
        {
            _dropHandle.setBounds(bounds.removeFromBottom(handleSize));
            break;
        }
        case DropLocation::parentLeft:
        case DropLocation::viewLeft:
        {
            _dropHandle.setBounds(bounds.removeFromLeft(handleSize));
            break;
        }
        case DropLocation::parentRight:
        case DropLocation::viewRight:
        {
            _dropHandle.setBounds(bounds.removeFromRight(handleSize));
            break;
        }
        case DropLocation::rootTop:
        {
            _dropHandle.setBounds(getLocalBounds().removeFromTop(handleSize));
            break;
        }
        case DropLocation::rootBottom:
        {
            _dropHandle.setBounds(getLocalBounds().removeFromBottom(handleSize));
            break;
        }
        case DropLocation::rootLeft:
        {
            _dropHandle.setBounds(getLocalBounds().removeFromLeft(handleSize));
            break;
        }
        case DropLocation::rootRight:
        {
            _dropHandle.setBounds(getLocalBounds().removeFromRight(handleSize));
            break;
        }
        case DropLocation::none:
        {
            break; 
        }
    }
}


void WindowComponent::hideDropHandle()
{
    _dropHandle.setVisible(false);
}





/**
 ===================================
 MARK: - Refresh -
 ===================================
 */
void WindowComponent::refresh()
{
    if (_dockingComponent)
        removeChildComponent(_dockingComponent.get());
    
    _dockingComponent = std::make_unique<DockingComponent>(_manager, _data, _tree.getChild(0));
    addAndMakeVisible(_dockingComponent.get());
    if (_overlay)
        _overlay->toFront(false);
    resized();
}





/**
 ===================================
 MARK: - Value Tree Listener -
 ===================================
 */

void WindowComponent::valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded)
{
    if (parentTree != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Added: WindowComp " << _data.getName(_tree) << " " << _data.getName(childWhichHasBeenAdded));
    
    /// Remove Component
    if (_dockingComponent != nullptr)
        removeChildComponent(_dockingComponent.get());
    
    /// Create New Docking Component
    _dockingComponent = std::make_unique<DockingComponent>(_manager, _data, childWhichHasBeenAdded);
    
    /// Add To View
    addAndMakeVisible(_dockingComponent.get());
    if (_overlay)
        _overlay->toFront(false);
    resized();
}


void WindowComponent::valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    if (parentTree != _tree.getParent()) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Removed: WindowComp");
    if (childWhichHasBeenRemoved == parentTree.getChild(0) && _dockingComponent != nullptr)
    {
        removeChildComponent(_dockingComponent.get()); 
        _dockingComponent.reset();
    }
}


void WindowComponent::valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged)
{
    if (treeWhoseParentHasChanged != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Parent Changed: WindowComp " << _data.getUuid(treeWhoseParentHasChanged) << " " << _data.getName(treeWhoseParentHasChanged));
}


void WindowComponent::valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex)
{
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Order Changed: WindowComp");
}


void WindowComponent::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    if (treeWhosePropertyHasChanged != _tree) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Property Changed: WindowComp");
    if (property.toString() == dockProps::lockedProperty)
    {
        auto locked = _data.isWindowLocked(_tree);
        _window.setAlwaysOnTop(locked);
        if (locked)
        {
            _lockedButton = std::make_unique<juce::ImageButton>();
            auto image = juce::ImageCache::getFromMemory(BinaryData::LockOn_png, BinaryData::LockOn_pngSize);
            _lockedButton->setImages(true, true, true,
                                     image, 1.0, juce::Colours::orange,     /// normal
                                     image, 0.5, juce::Colours::lightblue,  /// Over
                                     image, 0.8, juce::Colours::blue);      /// Down
            _lockedButton->onClick = [this] {_data.setWindowLocked(_data.getUuid(_tree), false);};
            addAndMakeVisible(_lockedButton.get());
        }
        else
        {
            _lockedButton = nullptr;
        }
        resized();
        repaint();
    }
}



/**
 ===================================
 MARK: - Drag and Drop -
 ===================================
 */

bool WindowComponent::isInterestedInDragSource(const SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.sourceComponent.get())
        return dynamic_cast<TabComponent*>(dragSourceDetails.sourceComponent.get()) != nullptr
            || dynamic_cast<HeaderComponent*>(dragSourceDetails.sourceComponent.get()) != nullptr;
    return false;
}


void WindowComponent::itemDragEnter(const SourceDetails &dragSourceDetails)
{
    itemDragMove(dragSourceDetails);
}


void WindowComponent::itemDragExit(const SourceDetails &dragSourceDetails)
{
    hideDropHandle();
}


void WindowComponent::itemDragMove(const SourceDetails &dragSourceDetails)
{
    auto location = getDragLocation(getMouseXYRelative());
    auto treeThatIsMoving = dragSourceDetails.description[0].toString();
    auto [treeToDropAt, index] = _data.getTreeForDockLocation(_data.getUuid(_tree), location);
    
//    DBG("Tree to Drop At: " << _data.getName(treeToDropAt)
//        <<", Dragging: " << treeThatIsMoving
//        << ", Location: " << _data.dropLocationToString(location)
//        << ", Point: " << localPointToGlobal(getMouseXYRelative()).toString());

    if (treeToDropAt == treeThatIsMoving || _data.getUuid(_tree) == treeThatIsMoving)
        hideDropHandle();
    else
        showDropHandleAt(treeToDropAt, index, location);
}


void WindowComponent::itemDropped(const SourceDetails &dragSourceDetails)
{
    /// Tell Manager not to create new view
    _manager.setCreateNewView(false);

    /// Hide  Drop Component
    hideDropHandle();
    
    /// Get Data
    auto location = getDragLocation(getMouseXYRelative());
    auto [treeToDropAt, index] = _data.getTreeForDockLocation(_data.getUuid(_tree), location);
    auto viewToDock = dragSourceDetails.description[0].toString();

//    DBG("Tree to Drop At: " << _data.getName(treeToDropAt)
//        <<", Dragging: " << _data.getName(viewToDock)
//        << ", Location: " << _data.dropLocationToString(location)
//        << ", Point: " << localPointToGlobal(getMouseXYRelative()).toString());
    
    /// Drop
    _data.dockView(viewToDock, treeToDropAt, location, localPointToGlobal(getMouseXYRelative()).toFloat());

}


const DropLocation WindowComponent::getDragLocation(const juce::Point<int>& position) const
{
    auto peerBounds = getLocalBounds();
    const int rootHitSize = 10;
    auto rootTop = peerBounds.removeFromTop(rootHitSize);
    auto rootBottom = peerBounds.removeFromBottom(rootHitSize);
    auto rootLeft = peerBounds.removeFromLeft(rootHitSize);
    auto rootRight = peerBounds.removeFromRight(rootHitSize);

    if (rootTop.contains(position))
        return DropLocation::rootTop;
    else if (rootBottom.contains(position))
        return DropLocation::rootBottom;
    else if (rootLeft.contains(position))
        return DropLocation::rootLeft;
    else if (rootRight.contains(position))
        return DropLocation::rootRight;
    else
        return DropLocation::none;
}





/**
 -------------------------------------------------------------
 ===================================
 MARK: - Docking Window -
 ===================================
 -------------------------------------------------------------
 */

DockingWindow::DockingWindow(DockManager& manager, DockManagerData& data, const juce::ValueTree& tree) : juce::DocumentWindow("", juce::Colours::lightgrey, juce::DocumentWindow::allButtons),  _manager(manager), _data(data), _tree(tree), _rootComponent(*this, _manager, _data, _tree)
{
    auto id = _data.getUuid(_tree);
    auto name = _data.getName(_tree);
    
    /// Check before Proceeding 
    checkWindowSize();
    
    auto position = _data.getPosition(_tree);
    auto size = _data.getSize(_tree);
    
    if (size.isOrigin())
        size.setXY(800, 800);

    /// Set content
    setContentNonOwned(&_rootComponent, true);
    
    /// Setup Window
    setName(_manager._delegate.getDefaultWindowName());
    setTopLeftPosition(position.getX(), position.getY());
    setSize(size.getX(), size.getY());
    setUsingNativeTitleBar(true);
    setResizable(true, false);
    setVisible(true);

    /// Add Listener
    if (auto listener = _manager._delegate.getKeyListenerForWindow(name))
        addKeyListener(listener);
}

DockingWindow::~DockingWindow()
{

}



/**
 ====================================
 MARK: - Resize -
 ====================================
 */

void DockingWindow::moved()
{
    juce::DocumentWindow::moved();
    _data.setPosition(_tree, getPosition().toFloat());
    
}


void DockingWindow::resized()
{
    juce::DocumentWindow::resized();
    _data.setWidth(_tree, getWidth());
    _data.setHeight(_tree, getHeight());
}




/**
 ====================================
 MARK: - Buttons Pressed -
 ====================================
 */

void DockingWindow::closeButtonPressed()
{
    auto id = _data.getUuid(_tree);
    _data.removeWindow(id);
}


void DockingWindow::minimiseButtonPressed()
{
    juce::DocumentWindow::minimiseButtonPressed();
    _data.setWindowMinimized(_data.getUuid(_tree), true);
    _data.setWindowMaximized(_data.getUuid(_tree), false);
}


void DockingWindow::maximiseButtonPressed()
{
    juce::DocumentWindow::maximiseButtonPressed();
    _data.setWindowMinimized(_data.getUuid(_tree), false);
    _data.setWindowMaximized(_data.getUuid(_tree), true);
}





/**
 ====================================
 MARK: - Checks -
 ====================================
 */

void DockingWindow::checkWindowSize()
{
    auto position = _data.getPosition(_tree);
    auto size = _data.getSize(_tree);
    auto displays = juce::Desktop::getInstance().getDisplays();
    auto display = displays.getDisplayForPoint(position.toInt());
    if (display == nullptr)
        display = displays.getPrimaryDisplay();
    
    if (display) 
    {
        auto area = display->userArea.withTrimmedTop(getTitleBarHeight());
        if (area.getRight() < position.getX() || position.getX() < area.getX())
            _data.setX(_tree, area.getX());
        
        if (area.getBottom() < position.getY() || position.getY() < area.getY())
            _data.setY(_tree, area.getY());
        
        if (area.getWidth() < size.getX() || size.isOrigin())
            _data.setWidth(_tree, area.getWidth());
        
        if (area.getHeight() < size.getY()|| size.isOrigin())
            _data.setHeight(_tree, area.getHeight());
    }
}



/**
 ====================================
 MARK: - Overlay -
 ====================================
 */
class Overlay : public juce::Component
{
public:
    Overlay(const juce::String& text) : _textToShow(text) {}
    
    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::grey.withAlpha(0.5f));
        
        g.setColour(juce::Colours::grey);
        g.fillRoundedRectangle(getLocalBounds().withSizeKeepingCentre(150, 30).toFloat(), 10);
        g.setColour(juce::Colours::white);
        g.drawText(_textToShow, getLocalBounds(), juce::Justification::centred);
    }
    
    juce::String _textToShow = "";
};


void WindowComponent::showOverlay(bool show, const juce::String& textToShow)
{
    if (show)
    {
        _overlay = std::make_unique<Overlay>(textToShow);
        addAndMakeVisible(_overlay.get());
        _overlay->toFront(false);
        resized();
    }
    else
        _overlay = nullptr; 
}

void DockingWindow::showOverlay(bool show, const juce::String& textToShow)
{
    _rootComponent.showOverlay(show, textToShow);
}
