#include "DockManager.h"
#include "DockingWindow.h"

/**
 -------------------------------------------------------------
 ===================================
 MARK: - Docking Window Manager -
 ===================================
 -------------------------------------------------------------
 */

DockManager::DockManager(Delegate& delegate) : _delegate(delegate)
{
    _data._rootTree.addListener(this);
#if JUCE_MAC
    _menu = _delegate.getMenuForWindow("");
    
#endif
}


DockManager::~DockManager()
{
    _components.clear();
    _windows.clear();
}



/**
 ===================================
 MARK: - Open In New Window -
 ===================================
 */

void DockManager::openViewInNewWindow(const juce::String& viewName, juce::Rectangle<float> bounds)
{
    _data.createInNewWindow(viewName, bounds);
}


/**
 ====================================
 MARK: - Open View As New Tab -
 ====================================
 */
void DockManager::openViewAsNewTab(const juce::String& viewName, const juce::String& regex, DropLocation fallbackType)
{
    _data.openViewAsNewTab(viewName, regex, fallbackType);
}





/**
 ===================================
 MARK: - Show View -
 ===================================
 */

bool DockManager::showView(const juce::String& viewName)
{
    return _data.showView(viewName);
}




/**
 ===================================
 MARK: - Reset Display Name -
 ===================================
 */

void DockManager::resetDisplayName(const juce::Component* component)
{
    if (component == nullptr) {return;}
    auto parent = component->findParentComponentOfClass<DockingComponent>();
    if (parent == nullptr) {return;}
    parent->resetDisplayName();
}


void DockManager::resetAllDisplayNames()
{
    for (auto window : _windows)
        window->resetAllDisplayNames();
}


/**
 ====================================
 MARK: - Layouts -
 ====================================
 */

void DockManager::saveLayout(const juce::File& fileToSave)
{
    _data.saveToFile(fileToSave);
}


void DockManager::saveLayout(juce::OutputStream& outputStream)
{
    _data.saveLayout(outputStream);
}


void DockManager::openLayout(const juce::File& fileToOpen)
{
    _components.clear();
    _windows.clear();
    _data.openFromFile(fileToOpen);
    
    for (auto window : _windows)
        window->layoutDidLoad();
}


void DockManager::openLayout(juce::InputStream& inputStream)
{
    _components.clear();
    _windows.clear();
    _data.openLayout(inputStream);
}


void DockManager::saveTemplate()
{
    _fileChooser = std::make_unique<juce::FileChooser>("Save Template",
                                                       juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory),
                                                       "*.xml");
    auto flags = juce::FileBrowserComponent::FileChooserFlags::saveMode;
    _fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser){
        auto file = chooser.getResult();
        if (file.getFileName().isEmpty()) {return;}
        _data.saveAsTemplate(file);
    });
}


void DockManager::openTemplate()
{
    _fileChooser = std::make_unique<juce::FileChooser>("Open Template",
                                                       juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory),
                                                       "*.xml");
    
    auto flags = juce::FileBrowserComponent::FileChooserFlags::openMode
                    | juce::FileBrowserComponent::FileChooserFlags::canSelectFiles;
    
    _fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser){
        auto file = chooser.getResult();
        if (!file.existsAsFile()) {return;}
        _data.openFromFile(file);
    });
}





/**
 ===================================
 MARK: - Presets -
 ===================================
 */

void DockManager::create2Up(const juce::String& windowName, const juce::StringArray& views)
{
    juce::Rectangle<float> bounds;
    if (auto display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        bounds = display->userArea.toFloat();
    
    auto [windowId, rootId] = _data.addNewWindow(windowName, bounds);
    if (views.isEmpty()) {return;}
    _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(0, 0, views.size() - 1)]);
    _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(1, 0, views.size() - 1)]);
}


void DockManager::create3Up(const juce::String& windowName, const juce::StringArray& views)
{
    juce::Rectangle<float> bounds;
    if (auto display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        bounds = display->userArea.toFloat();
    
    auto [windowId, rootId] = _data.addNewWindow(windowName, bounds);
    if (views.isEmpty()) {return;}
    _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(0, 0, views.size() - 1)]);
    _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(1, 0, views.size() - 1)]);
    _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(2, 0, views.size() - 1)]);
}


void DockManager::create4Up(const juce::String& windowName, const juce::StringArray& views)
{
    juce::Rectangle<float> bounds;
    if (auto display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        bounds = display->userArea.toFloat();
    
    auto [windowId, rootId] = _data.addNewWindow(windowName, bounds);
    if (views.isEmpty()) {return;}
    _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(0, 0, views.size() - 1)]);
    _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(1, 0, views.size() - 1)]);
    _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(2, 0, views.size() - 1)]);
    _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(3, 0, views.size() - 1)]);
}

 
void DockManager::create2By2(const juce::String& windowName, const juce::StringArray& views)
{
    juce::Rectangle<float> bounds;
    if (auto display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        bounds = display->userArea.toFloat();

    auto [windowId, rootId] = _data.addNewWindow(windowName, bounds);
    if (views.isEmpty()) {return;}
    auto leftColumn = _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(0, 0, views.size() - 1)]);
    auto rightColumn = _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(2, 0, views.size() - 1)]);
    _data.dockNewView(leftColumn, DropLocation::viewBottom, views[std::clamp<int>(1, 0, views.size() - 1)]);
    _data.dockNewView(rightColumn, DropLocation::viewBottom, views[std::clamp<int>(3, 0, views.size() - 1)]);

}


void DockManager::create3By3(const juce::String& windowName, const juce::StringArray& views)
{
    juce::Rectangle<float> bounds;
    if (auto display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        bounds = display->userArea.toFloat();
    
    auto [windowId, rootId] = _data.addNewWindow(windowName, bounds);
    if (views.isEmpty()) {return;}
    auto leftColumn = _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(0, 0, views.size() - 1)]);
    auto centerColumn = _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(2, 0, views.size() - 1)]);
    auto rightColumn = _data.dockNewView(rootId, DropLocation::rootRight, views[std::clamp<int>(4, 0, views.size() - 1)]);
    auto leftBottom = _data.dockNewView(leftColumn, DropLocation::viewBottom, views[std::clamp<int>(1, 0, views.size() - 1)]);
    auto centerBottom = _data.dockNewView(centerColumn, DropLocation::viewBottom, views[std::clamp<int>(3, 0, views.size() - 1)]);
    auto rightBottom = _data.dockNewView(rightColumn, DropLocation::viewBottom, views[std::clamp<int>(5, 0, views.size() - 1)]);
}


void DockManager::create2Rows(const juce::String& windowName, const juce::StringArray& views)
{
    juce::Rectangle<float> bounds;
    if (auto display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        bounds = display->userArea.toFloat();
    
    auto [windowId, rootId] = _data.addNewWindow(windowName, bounds);
    if (views.isEmpty()) {return;}
    _data.dockNewView(rootId, DropLocation::rootBottom, views[std::clamp<int>(0, 0, views.size() - 1)]);
    _data.dockNewView(rootId, DropLocation::rootBottom, views[std::clamp<int>(1, 0, views.size() - 1)]);
}


void DockManager::create3Rows(const juce::String& windowName, const juce::StringArray& views)
{
    juce::Rectangle<float> bounds;
    if (auto display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        bounds = display->userArea.toFloat();
    
    auto [windowId, rootId] = _data.addNewWindow(windowName, bounds);
    if (views.isEmpty()) {return;}
    _data.dockNewView(rootId, DropLocation::rootBottom, views[std::clamp<int>(0, 0, views.size() - 1)]);
    _data.dockNewView(rootId, DropLocation::rootBottom, views[std::clamp<int>(1, 0, views.size() - 1)]);
    _data.dockNewView(rootId, DropLocation::rootBottom, views[std::clamp<int>(2, 0, views.size() - 1)]);
}




/**
 ====================================
 MARK: - Remove View -
 ====================================
 */

void DockManager::removeView(const juce::String& viewId)
{
    _data.removeView(viewId);
    
    if (_components.contains(viewId))
        _components.remove(viewId);
}





/**
 ===================================
 MARK: - Components -
 ===================================
 */

std::shared_ptr<juce::Component> DockManager::getComponent(const juce::String& uuid, const juce::String& name)
{
    if (_components.contains(uuid))
        return _components[uuid];
    
    /// Create a new view
    auto newView = _delegate.createView(name);
    if (!newView) {return nullptr;}
    
    /// Add to Stored Views
    _components.set(uuid, newView);
    
    /// Return Component
    return _components[uuid];
}




/**
 ===================================
 MARK: - Menus -
 ===================================
 */

juce::PopupMenu DockManager::getHeaderPopupMenu(const juce::ValueTree& tree)
{
    juce::PopupMenu menu;
    
    menu.addSubMenu("Add Tab", getAddViewPopupMenu(tree, DropLocation::tabs));
    menu.addSubMenu("Add View", getAddViewAtPopupMenu(tree));
    
    menu.addSeparator();
    menu.addItem("Close View", [tree, this]{
        removeView(_data.getUuid(tree));
    });
    menu.addSeparator();
    menu.addItem("Open in New Window", [tree, this]{
        openInNewWindow(tree);
    });
    menu.addItem(_data.isWindowLocked(tree) ? "Unlock Window" : "Lock Window", [tree, this]{
        _data.setWindowLocked(_data.getUuid(_data.findWindow(tree)), !_data.isWindowLocked(tree)); 
    });
    
#if JUCE_DEBUG
    menu.addSeparator();
    juce::PopupMenu debugMenu;
    debugMenu.addItem("Save As Template", [this]{
        saveTemplate();
    });
    debugMenu.addItem("Open Template", [this]{
        openTemplate();
    });
    debugMenu.addSeparator();
    debugMenu.addItem("Test New Tree", [this]
    {
        openViewAsNewTab("Canvas:12344", "Canvas(?!List)", DropLocation::rootLeft);
    });
    debugMenu.addSeparator();
    debugMenu.addItem("Print Tree", [this]{
        _data.printTree();
    });
    menu.addSubMenu("Debug", debugMenu);
#endif
    return menu;
}


juce::PopupMenu DockManager::getTabPopupMenu(const juce::ValueTree& tree)
{
    juce::PopupMenu menu;
    menu.addSubMenu("Add Tab", getAddViewPopupMenu(tree, DropLocation::tabs));
    menu.addSubMenu("Add View", getAddViewAtPopupMenu(tree));
    menu.addSeparator();
    menu.addItem("Open In New Window", [tree, this] {
        openInNewWindow(tree);
    });
    menu.addSeparator();
    menu.addItem("Close Tab", [tree, this]{
        removeView(_data.getUuid(tree));
    });
    menu.addItem("Close Other Tabs", [tree, this]{
        auto parent = tree.getParent();
        if (!parent.isValid()) {return;}
        for (auto i = 0; i < parent.getNumChildren(); i++)
        {
            auto child = parent.getChild(i);
            if (child.isValid() && child != tree)
                removeView(_data.getUuid(child));
        }
    });
    
#if JUCE_DEBUG
    juce::PopupMenu debugMenu;
    menu.addSeparator();
    debugMenu.addSeparator();
    debugMenu.addItem("Save As Template", [this]{
        saveTemplate();
    });
    debugMenu.addItem("Open Template", [this]{
        openTemplate();
    });
    debugMenu.addSeparator();
    debugMenu.addItem("Print Tree", [this]{
        _data.printTree();
    });
    debugMenu.addSubMenu("Debug", debugMenu);
#endif
    return menu;
}


juce::PopupMenu DockManager::getAddViewAtPopupMenu(const juce::ValueTree& tree)
{
    juce::PopupMenu addViewMenu;
    
    for (const auto& view : _delegate.getAvailableViews())
    {
        juce::PopupMenu viewMenu;
            for (auto i = 0; i < 14; i++)
            {
                auto location = DropLocation(i);
                if (_data.isParentDropLocation(location)) {continue;}
                viewMenu.addItem(_data.dropLocationToString(location), [tree, location, view, this]{
                    if (location == DropLocation::none)
                        openViewInNewWindow(view, {});
                    else
                        _data.dockNewView(_data.getUuid(tree), location, view);
                });
            }
        addViewMenu.addSubMenu(view, viewMenu);
    }
    
    return addViewMenu;
}


juce::PopupMenu DockManager::getAddViewPopupMenu(const juce::ValueTree& tree, DropLocation location)
{
    juce::PopupMenu addViewMenu;
    const auto& availableViews = _delegate.getAvailableViews();
    for (const auto& view : availableViews)
    {
        if (location == DropLocation::none)
            addViewMenu.addItem(view, [tree, view, location, this]
            {
                openViewInNewWindow(view, {});
            });

        else
            addViewMenu.addItem(view, [tree, view, location, this]
            {
                _data.dockNewView(_data.getUuid(tree), location, view);
            });
    }
    
    return addViewMenu;
}


void DockManager::openInNewWindow(const juce::ValueTree& tree)
{
    juce::Rectangle<float> bounds;
    const auto uuid = _data.getUuid(tree);
    auto mousePosition = juce::Desktop::getInstance().getMousePosition().toFloat();
    
    if (_components.contains(uuid))
        bounds = _components[uuid]->getBounds().withPosition(mousePosition.x, mousePosition.y).toFloat();
    _data.openInNewWindow(_data.getUuid(tree), {}, bounds);
}



/**
 ===================================
 MARK: - Utility -
 ===================================
 */

void DockManager::printTree()
{
    _data.printTree();
}



/**
 ====================================
 MARK: - Throttler -
 ====================================
 */
class DockManager::UpdateThrottler : public juce::Timer
{
public:
    UpdateThrottler(DockManager& manager) : _manager(manager) {}
    void didRecieveUpdate() { startTimer(3000); }
private:
    void timerCallback() override {_manager._delegate.didUpdateLayouts(); stopTimer();}
    DockManager& _manager;
};


/**
 ===================================
 MARK: - Value Tree Listener -
 ===================================
 */

void DockManager::valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded)
{
    if (parentTree != _data.getTree()) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Added");
    
    /// Get Id
    auto id = _data.getUuid(childWhichHasBeenAdded);
    
    /// Create Window
    auto window = std::make_shared<DockingWindow>(*this, _data, childWhichHasBeenAdded);
    
    /// Add to Map
    _windows.set(id, window);
}


void DockManager::valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    if (parentTree != _data.getTree()) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Removed");
    auto id = _data.getUuid(childWhichHasBeenRemoved);
    _windows.remove(id);
}


void DockManager::valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged)
{
    if (treeWhoseParentHasChanged != _data.getTree()) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Child Parent Changed");
}


void DockManager::valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex)
{
    if (parentTreeWhoseChildrenHaveMoved != _data.getTree()) {return;}
    if (PRINT_TREE_LISTENERS) DBG("Value Tree Order Changed");
}


void DockManager::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    if (_throttler == nullptr)
        _throttler = std::make_unique<DockManager::UpdateThrottler>(*this);
    _throttler->didRecieveUpdate();
    
    if (treeWhosePropertyHasChanged != _data.getTree()) {return;}
}



/**
 ===================================
 MARK: - Drag and Drop helper -
 ===================================
 */

void DockManager::setCreateNewView(bool createNewView)
{
    _createNewView = createNewView;
}


void DockManager::createNewWindow(const juce::String& withViewId, const juce::Point<float>& atPosition)
{
    if (!_createNewView) {return;}
    setCreateNewView(false);
    _data.openInNewWindow(withViewId, atPosition);
}






/**
 ====================================
 MARK: - Overlay -
 ====================================
 */

void DockManager::showOverlayWithText(bool show, const juce::String& textToShow)
{
    for (auto window : _windows)
        window->showOverlay(show, textToShow); 
}





/**
 ====================================
 MARK: - Get All Components -
 ====================================
 */

const DockManager::ViewMap& DockManager::getAllComponents() const
{
    return _components; 
}


juce::Component* DockManager::getCurrentlyFocusedComponent() const
{
    for (auto comp : _components)
        if (comp->hasKeyboardFocus(true)
            || (comp->getParentComponent() != nullptr && comp->getParentComponent()->hasKeyboardFocus(true)))
            return comp.get();
    
    return nullptr;
}


