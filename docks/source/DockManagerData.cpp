#include "DockManagerData.h"
#include <regex>

DockManagerData::DockManagerData()
{
    setLayoutName("Current Layout"); 
}


DockManagerData::~DockManagerData()
{
    
}





/**
 ===================================
 MARK: - Save/Open -
 ===================================
 */

bool DockManagerData::saveAsTemplate(const juce::File& file)
{
    auto fileToSave = file;
    
    if (fileToSave.getFileExtension() != "xml")
        fileToSave.withFileExtension("xml");
    
    if (!fileToSave.existsAsFile())
        fileToSave.create();
    
    auto treeToSave = _rootTree.createCopy();
    
    /// Set the Template name as the root
    treeToSave.setProperty(dockProps::nameProperty, file.getFileNameWithoutExtension(), nullptr);
    
    /// Remove Window properties.
    for (auto child : treeToSave)
    {
        child.removeProperty(dockProps::xProperty, nullptr);
        child.removeProperty(dockProps::yProperty, nullptr);
        child.removeProperty(dockProps::widthProperty, nullptr);
        child.removeProperty(dockProps::heightProperty, nullptr);
        child.removeProperty(dockProps::windowMinimized, nullptr);
        child.removeProperty(dockProps::windowMaximized, nullptr);
    }
    
    
    auto xml = treeToSave.createXml();
    if (!xml) {return false;}
    return xml->writeTo(fileToSave);
}


bool DockManagerData::saveToFile(const juce::File& file)
{
    if (!file.existsAsFile())
        file.create();
        
    auto xml = _rootTree.createXml();
    if (!xml) {return false;}
    return xml->writeTo(file);
}


bool DockManagerData::saveLayout(juce::OutputStream& outputStream)
{
    auto xml = _rootTree.createXml();
    if (!xml) {return false;}
    xml->writeTo(outputStream);
    return true;
}


bool DockManagerData::openFromFile(const juce::File& file)
{
    if (!file.existsAsFile()) {return false;}
    auto xml = juce::parseXML(file);
    if (!xml) {return false;}
    _rootTree.removeAllChildren(nullptr);
    _rootTree.copyPropertiesAndChildrenFrom(juce::ValueTree::fromXml(*xml), nullptr);
    return true;
}


bool DockManagerData::openLayout(juce::InputStream& inputStream)
{
    auto xml = juce::parseXML(inputStream.readEntireStreamAsString());
    if (!xml) {return false;}
    _rootTree.removeAllChildren(nullptr);
    _rootTree.copyPropertiesAndChildrenFrom(juce::ValueTree::fromXml(*xml), nullptr);
    return true;
}





/**
 ===================================
 MARK: - Windows -
 ===================================
 */

const std::pair<juce::String, juce::String> DockManagerData::addNewWindow(const juce::String& named, const juce::Rectangle<float> bounds)
{
    auto windowTree = juce::ValueTree(dockIds::windowIdentifier);
    auto uuid = juce::Uuid().toString();
    
    windowTree.setProperty(dockProps::uuidProperty, uuid, nullptr);
    if (!bounds.isEmpty())
        setBounds(windowTree, bounds);
    
    setName(windowTree, named);
    
    /// Add to Root Tree
    _rootTree.addChild(windowTree, -1, nullptr);
    
    /// Add Root View
    auto rootId = addRootView(uuid);
    
    /// Return the new Window Id
    return {uuid, rootId};
}


void DockManagerData::removeWindow(const juce::String& windowId)
{
    auto windowTree = findTree(windowId);
    if (!isWindow(windowTree)) {return;}
    _rootTree.removeChild(windowTree, nullptr);
}


void DockManagerData::clearWindows()
{
    _rootTree.removeAllChildren(nullptr);
}


const juce::String DockManagerData::addRootView(const juce::String& toWindow)
{
    auto window = findTree(toWindow);
    if (!window.isValid()) {return "";}
    auto id = juce::Uuid().toString();
    auto tree = juce::ValueTree(dockIds::rootTreeIdentifier);
    tree.setProperty(dockProps::uuidProperty, id, nullptr);
    setName(tree, dockIds::rootTreeIdentifier);
    setDockType(tree, DockTypes::none);
    
    /// Add Child
    window.addChild(tree, -1, nullptr);
    
    /// Return Id
    return id;
}



/**
 ===================================
 MARK: - Views -
 ===================================
 */

juce::ValueTree DockManagerData::getNewView(const juce::String viewName, DockTypes dockAt)
{
    auto viewId = juce::Uuid().toString();
    auto viewTree = juce::ValueTree(dockIds::viewIdentifier);
    viewTree.setProperty(dockProps::uuidProperty, viewId, nullptr);
    setName(viewTree, viewName.isEmpty() ? getRandomName() : viewName);
    setDockType(viewTree, dockAt);
    return viewTree;
}


const juce::String DockManagerData::addView(const juce::String& toView, const juce::String& viewName, DockTypes dockAt)
{
    auto treeToAddTo = findTree(toView);
    if (isWindow(treeToAddTo)) {return "";}
    
    /// Get New View
    auto viewTree = getNewView(viewName, dockAt);
    
    /// Add to Window
    treeToAddTo.addChild(viewTree, -1, nullptr);
    
    /// Return the View Id;
    return getUuid(viewTree);
}


void DockManagerData::removeView(const juce::String& viewid)
{
    auto view = findTree(viewid);
    if (!isView(view)) {return;}
    auto viewParent = view.getParent();
    if (!viewParent.isValid()) {return;}
    viewParent.removeChild(view, nullptr);

    /// Check for Selected Tab
    if (getDockType(viewParent) == DockTypes::tabs
        && getSelectedId(viewParent) == viewid
        && viewParent.getNumChildren() > 1)
        setSelected(viewParent, getUuid(viewParent.getChild(0)));
    
    checkForOrphanedTrees();
}


void DockManagerData::removeViewAndChildren(const juce::String& viewid)
{
    auto view = findTree(viewid);
    if (!isView(view)) {return;}
    auto viewParent = view.getParent();
    if (!viewParent.isValid()) {return;}
    view.removeAllChildren(nullptr);
    viewParent.removeChild(view, nullptr);
    
    /// Check for Selected Tab
    if (getDockType(viewParent) == DockTypes::tabs
        && getSelectedId(viewParent) == viewid
        && viewParent.getNumChildren() > 1)
        setSelected(viewParent, getUuid(viewParent.getChild(0)));
    
    checkForOrphanedTrees();
}


bool DockManagerData::showView(const juce::String& viewName)
{
    auto tree = findTree(dockProps::nameProperty, viewName, _rootTree);
    if (!tree.isValid()) {return false;}
    if (getDockType(tree.getParent()) == DockTypes::tabs)
    {
        auto parent = tree.getParent();
        if (!parent.isValid()) {return false;}
        setSelected(parent, getUuid(tree));
        return true;
    }
    return false;
    
}


/**
 ===================================
 MARK: - Docking -
 ===================================
 */

const bool DockManagerData::canDock(const juce::String& viewToDockIn, DropLocation location) const
{
    return true;
}


void DockManagerData::openInNewWindow(const juce::String& treeId, const juce::Point<float>& position, const juce::Rectangle<float>& bounds)
{
    auto tree = findTree(treeId);
    if (!tree.isValid()) {return;}
    removeChildFromParent(tree);
    dockInNewWindow(tree, position, bounds);
}


void DockManagerData::createInNewWindow(const juce::String& viewName, const juce::Rectangle<float>& bounds)
{
    auto tree = getNewView(viewName, DockTypes::none);
    if (tree.isValid())
        dockInNewWindow(tree, {}, bounds);
}


void DockManagerData::openViewAsNewTab(const juce::String& viewName, const juce::String& regex, DropLocation fallbackType)
{
    auto expr = std::regex(regex.toStdString());
    auto treeToOpenIn = findTree(_rootTree, [expr](const juce::ValueTree& tree)->bool {
        if (!tree.hasProperty(dockProps::nameProperty)) {return false;}
        auto name = tree.getProperty(dockProps::nameProperty).toString().toStdString();
        return std::regex_match(name.begin(), name.end(), expr);
    });
    
    if (treeToOpenIn.isValid())
    {
        /// Open as Tab
        dockNewView(getUuid(treeToOpenIn), DropLocation::tabs, viewName);
    }
    else
    {
        /// Open as fallback
        auto firstWindow = getRootTreeForWindow(getUuid(_rootTree.getChild(0)));
        dockNewView(getUuid(firstWindow), fallbackType, viewName);
    }
}


const juce::String DockManagerData::dockNewView(const juce::String& viewToDockIn, DropLocation location, const juce::String& name)
{
    if (!canDock(viewToDockIn, location)) {return "";}
    
    /// Create New Tree
    auto treeToDock = getNewView(name, DockTypes::none);
    auto newTreeId = getUuid(treeToDock);
    
    /// Get the Tree to dock in
    auto treeToDockIn = isRootDropLocation(location) ? getRootView(viewToDockIn) : findTree(viewToDockIn);
        
    /// Index
    auto index = getIndexForLocation(location);

    /// Dock view
    auto status = dockView(treeToDock, treeToDockIn, location, index);
    
    /// Return new tree
    return status ? newTreeId : "";
}


void DockManagerData::dockView(const juce::String& viewToDock, const juce::String& viewToDockIn, DropLocation location, juce::Point<float> dropPosition, int index)
{
    if (!canDock(viewToDockIn, location) || viewToDock == viewToDockIn) {return;}
    
    /// Create New Tree
    auto treeToDock = findTree(viewToDock);
    
    /// Get the Tree to dock in
    auto treeToDockIn = findTree(viewToDockIn);
    
    /// Index
    auto newIndex = location == DropLocation::tabs ? index : getIndexForLocation(location);
    
    /// Remove From Parent
    removeChildFromParent(treeToDock);
    
    /// Dock the Tree
    auto status = dockView(treeToDock, treeToDockIn, location, newIndex, dropPosition);
    if (!status)
        DBG("Could Not Drop: " << getName(viewToDock) << " In: " << getName(viewToDockIn));
}


bool DockManagerData::dockView(const juce::ValueTree& treeToDock, const juce::ValueTree& treeToDockIn, DropLocation location, int index, juce::Point<float> dropPosition)
{
    /// Try to dock in view recieved
    DBG("Docking: " << dropLocationToString(location));
    auto status = dockInView(treeToDock, treeToDockIn, location, index);
    if (status) {/**DBG("Did Dock in View!"); */return true;}
    
    /// Try to dock in a parent
    status = dockInParent(treeToDock, treeToDockIn, location, index);
    if (status) {/**DBG("Did Dock In Parent!"); */return true;}
    
    /// Try to dock in root
    status = dockInRoot(treeToDock, treeToDockIn, location, index);
    if (status) {/**DBG("Did Dock In Root!"); */return true;}
    
    /// Try docking in new Window
    status = dockInNewWindow(treeToDock, dropPosition);
    if (status) {/**DBG("Dock In New Window!"); */return true;}
    
//    DBG("Could not dock anywhere...ViewToDock: " << getName(treeToDock) << ", ViewToDockIn: " << getName(treeToDockIn));
    return false; 
}


bool DockManagerData::dockInView(juce::ValueTree treeToDock, juce::ValueTree treeToDockIn, DropLocation dockLocation, int index)
{
    /// Get the type of docking requirement
    auto dockType = getTypeForLocation(dockLocation);

    if (isRootTree(treeToDockIn) || getDockType(treeToDockIn) != dockType || isRootDropLocation(dockLocation) || dockLocation == DropLocation::none) {return false;}
    
    /// Get the index
    int indexAdd = dockLocation == DropLocation::viewRight || dockLocation == DropLocation::viewBottom || dockLocation == DropLocation::parentRight || dockLocation == DropLocation::parentBottom ? 1 : 0;
    bool isTabs = dockLocation == DropLocation::tabs;
    
    /// Set index
    auto newIndex = treeToDockIn.getParent().indexOf(treeToDockIn) + indexAdd;
    
    /// Dock In View
    treeToDockIn.addChild(treeToDock, isTabs ? index : newIndex, nullptr);
    
    /// Set Selected Tab
    if (isTabs)
        setSelected(treeToDockIn, getUuid(treeToDock));
    
    /// Check for Orphans
    checkForOrphanedTrees();

    /// Return success
    return true;
}


bool DockManagerData::dockInParent(juce::ValueTree treeToDock, juce::ValueTree treeToDockIn, DropLocation dockLocation, int index)
{
    if (isRootTree(treeToDockIn) || treeToDock == treeToDockIn || isRootDropLocation(dockLocation) || dockLocation == DropLocation::none) {return false;}
    
    /// Get the type of docking requirement
    auto dockType = getTypeForLocation(dockLocation);

    /// Find a parent to dock In
    auto dropParentIsDockType = getDockType(treeToDockIn.getParent()) == dockType;
    auto parentToDockIn = isParentDropLocation(dockLocation) ? getParentForDropType(treeToDockIn.getParent(), dockType)
                            : dropParentIsDockType ? treeToDockIn.getParent()
                            : juce::ValueTree();
        
    if ((!parentToDockIn.isValid() && !dropParentIsDockType) || isWindow(parentToDockIn) || isRootTree(parentToDockIn))
    {
        /// Check that there is a place to dock the view.
        auto newTree = addAndMoveTree(treeToDockIn, dockType);
        if (newTree.isValid())
            newTree.addChild(treeToDock, index, nullptr);
                
        /// Set Selected Tab
        if (dockLocation == DropLocation::tabs)
            setSelected(newTree, getUuid(treeToDock));

        /// Check for Orphans
        checkForOrphanedTrees();
        
        return true;
    }
    else
    {
        /// Get the new Index
        auto indexAdd = dockLocation == DropLocation::viewRight || dockLocation == DropLocation::viewBottom
                        || dockLocation == DropLocation::parentRight || dockLocation == DropLocation::parentBottom ? 1 : 0;
        auto newIndex = treeToDockIn.getParent().indexOf(treeToDockIn) + indexAdd;
        
        /// Dock the child at the index
        parentToDockIn.addChild(treeToDock, newIndex, nullptr);

        /// Set Selected Tab
        if (dockLocation == DropLocation::tabs)
            setSelected(parentToDockIn, getUuid(treeToDock));

        /// Check for Orphans
        checkForOrphanedTrees();
        
        return true;
    }
    
    return false;
}


bool DockManagerData::dockInRoot(juce::ValueTree treeToDock, juce::ValueTree treeToDockIn, DropLocation dockLocation, int index)
{
    if ((!isRootDropLocation(dockLocation) && !isRootTree(treeToDockIn)) || dockLocation == DropLocation::none) {return false;}
    
    /// Get the type of docking requirement
    auto dockType = getTypeForLocation(dockLocation);
    
    /// Add new Spliter view
    juce::ValueTree newView;
    if (treeToDockIn.getNumChildren() == 0)
    {
        auto viewId = addView(getUuid(treeToDockIn), "", dockType);
        newView = findTree(viewId);
    }
    else if (getDockType(treeToDockIn.getChild(0)) == dockType)
    {
        /// If the first Tree in the Root is the type we wnat to use, dock in that type
        newView = treeToDockIn.getChild(0);
    }
    else
    {
        newView = addAndMoveTree(treeToDockIn.getChild(0), dockType);
    }
    
    if (!newView.isValid()) {return false;}
    
    /// Add View to new View
    newView.addChild(treeToDock, index, nullptr);
    
    /// Check for Orphans
    checkForOrphanedTrees();
    
    /// Add tree to new view
    return true;
}


bool DockManagerData::dockInNewWindow(juce::ValueTree treeToDock, juce::Point<float> dropPosition, juce::Rectangle<float> windowBounds)
{
    if (!treeToDock.isValid()) {return false;}
    auto [window, rootId] = addNewWindow("New Window", windowBounds);
    auto rootView = getRootTreeForWindow(window);
    if (!rootView.isValid()) {return false;}
    
    /// Add to root view for window
    rootView.addChild(treeToDock, -1, nullptr);
    auto size = getSize(window);
    setPosition(window, dropPosition.withX(dropPosition.getX() - size.getX()/2));
    
    /// Check for Orphans
    checkForOrphanedTrees();
    
    return true;
}



/**
 ===================================
 MARK: - Check For Orphans -
 ===================================
 */

void DockManagerData::checkForOrphanedTrees()
{
    checkForOrphanedTreesIn(_rootTree);
    checkForOrphanedWindows();
}


void DockManagerData::checkForOrphanedTreesIn(juce::ValueTree tree)
{
    for (auto i = 0; i < tree.getNumChildren(); i++)
    {
        auto child = tree.getChild(i);
        if (!child.isValid()) {continue;}
        auto shouldRemoveEmptyDockType = getDockType(child) != DockTypes::none
                                            && !isRootTree(child)
                                            && child.getNumChildren() == 1
                                            && getName(child).isEmpty();
        
        auto shouldRemoveEmptyTab = getDockType(child) == DockTypes::tabs
                                    && child.getNumChildren() == 0;
                
        if (shouldRemoveEmptyDockType || shouldRemoveEmptyTab)
        {
            auto treeToMove = child.getChild(0);
            auto index = tree.indexOf(child);
            child.removeChild(treeToMove, nullptr);
            tree.removeChild(child, nullptr);
            tree.addChild(treeToMove, index, nullptr);
            if (child.hasProperty(dockProps::widthProperty))
                setWidth(treeToMove, getWidth(child));
            if (child.hasProperty(dockProps::heightProperty))
                setHeight(treeToMove, getHeight(child));
        }
        
        else
            checkForOrphanedTreesIn(child);
    }
}


void DockManagerData::checkForOrphanedWindows()
{
    for (auto i = 0; i < _rootTree.getNumChildren(); i++)
    {
        auto child = _rootTree.getChild(i);
        if (!child.isValid()) {continue;}
        if (child.getNumChildren() == 0 || child.getChild(0).getNumChildren() == 0)
            _rootTree.removeChild(child, nullptr);
    }
}



/**
 ===================================
 MARK: - Add And Move Tree -
 ===================================
 */

juce::ValueTree DockManagerData::addAndMoveTree(const juce::ValueTree& tree, DockTypes type)
{
    if (!tree.isValid()) {return tree;}
    auto parent = tree.getParent();
    if (!parent.isValid()) {return tree;}
    auto index = parent.indexOf(tree);
    parent.removeChild(tree, nullptr);
    auto newViewTree = getNewView("", type);
    parent.addChild(newViewTree, index, nullptr);
    newViewTree.addChild(tree, -1, nullptr);
    return newViewTree;
}






/**
 ===================================
 MARK: - Utility -
 ===================================
 */

juce::ValueTree DockManagerData::getTree()
{
    return _rootTree;
}


void DockManagerData::printTree()
{
    DBG(_rootTree.toXmlString());
}





/**
 ===================================
 MARK: - Getters -
 ===================================
 */

const juce::String DockManagerData::getUuid(const juce::ValueTree& tree) const
{
    if (!tree.isValid())
        return "";
    
    return tree.getProperty(dockProps::uuidProperty);
}


const juce::Rectangle<float> DockManagerData::getBounds(const juce::String& uuid) const
{
    const auto tree = findTree(uuid);
    if (!tree.isValid()) {return {};}
    return getBounds(tree);
}


const juce::Rectangle<float> DockManagerData::getBounds(const juce::ValueTree& fromTree) const
{
    auto position = getPosition(fromTree);
    auto size = getSize(fromTree);
    return {position.x, position.y, size.x, size.y};
}


const juce::Point<float> DockManagerData::getPosition(const juce::String& uuid) const
{
    const auto tree = findTree(uuid);
    if (!tree.isValid()) {return {};}
    return getPosition(tree);
}


const juce::Point<float> DockManagerData::getPosition(const juce::ValueTree& fromTree) const
{
    auto x = getProperty<float>(fromTree, dockProps::xProperty);
    auto y = getProperty<float>(fromTree, dockProps::yProperty);
    return {x, y};
}


const juce::Point<float> DockManagerData::getSize(const juce::String& uuid) const
{
    const auto tree = findTree(uuid);
    if (!tree.isValid()) {return {};}
    return getSize(tree);
}


const juce::Point<float> DockManagerData::getSize(const juce::ValueTree& fromTree) const
{
    auto width = getWidth(fromTree);
    auto height = getHeight(fromTree);
    return {width, height};
}

const float DockManagerData::getWidth(const juce::ValueTree& tree) const
{
    return getProperty<float>(tree, dockProps::widthProperty);
}


const float DockManagerData::getHeight(const juce::ValueTree& tree) const
{
    return getProperty<float>(tree, dockProps::heightProperty);
}


const juce::String DockManagerData::getName(const juce::String& uuid) const
{
    const auto tree = findTree(uuid);
    if (!tree.isValid()) {return {};}
    return getName(tree);
}


const juce::String DockManagerData::getName(const juce::ValueTree& fromTree) const
{
    return getProperty<juce::String>(fromTree, dockProps::nameProperty);
}


const DockTypes DockManagerData::getDockType(const juce::String& uuid) const
{
    const auto tree = findTree(uuid);
    if (!tree.isValid()) {return DockTypes::none;}
    return getDockType(tree);
}


const DockTypes DockManagerData::getDockType(const juce::ValueTree& fromTree) const
{
    auto type = getProperty<int>(fromTree, dockProps::dockType);
    return DockTypes(type);
}


const juce::String DockManagerData::getSelectedId(const juce::ValueTree& tree) const
{
    if (tree.hasProperty(dockProps::selectedProperty))
        return getProperty<juce::String>(tree, dockProps::selectedProperty);
    else
        return "";
}


const bool DockManagerData::isSelected(const juce::ValueTree& tree) const
{
    return getUuid(tree) == getSelectedId(tree.getParent());
}


const bool DockManagerData::isWindowLocked(const juce::ValueTree& tree) const
{
    auto window = findWindow(tree);
    if (!window.isValid() || !isWindow(window)) {return false;}
    return getProperty<bool>(window, dockProps::lockedProperty);
}


template <typename T>
const T DockManagerData::getProperty(const juce::ValueTree& tree, const juce::String& propId) const
{
    if (!tree.isValid() || !tree.hasProperty(propId)) {return T();}
    T property = tree.getProperty(propId);
    return property;
}


const juce::String DockManagerData::dropLocationToString(DropLocation drop) const
{
    switch (drop)
    {
        case DropLocation::viewLeft:     {return "View Left";}
        case DropLocation::viewRight:    {return "View Right";}
        case DropLocation::viewTop:      {return "View Top";}
        case DropLocation::viewBottom:   {return "View Bottom";}
        case DropLocation::parentLeft:   {return "Parent Left";}
        case DropLocation::parentRight:  {return "Parent Right";}
        case DropLocation::parentTop:    {return "Parent Top";}
        case DropLocation::parentBottom: {return "Parent Bottom";}
        case DropLocation::tabs:         {return "Tabs";}
        case DropLocation::rootTop:      {return "Window Top";}
        case DropLocation::rootBottom:   {return "Window Bottom";}
        case DropLocation::rootLeft:     {return "Window Left";}
        case DropLocation::rootRight:    {return "Window Right";}
        case DropLocation::none:         {return "Window";}
    }
    return "";
}


const juce::String DockManagerData::dockTypeToString(DockTypes type) const
{
    switch (type) {
        case DockTypes::vertical:   return "Vertical";
        case DockTypes::horizontal: return "Horizontal";
        case DockTypes::tabs:       return "Tabs";
        case DockTypes::none:       return "None";
    }
    return "";
}


const bool DockManagerData::isParentDropLocation(DropLocation drop) const
{
    return drop == DropLocation::parentTop || drop == DropLocation::parentBottom || drop == DropLocation::parentLeft || drop == DropLocation::parentRight;
}


const bool DockManagerData::isRootDropLocation(DropLocation drop) const
{
    return drop == DropLocation::rootTop || drop == DropLocation::rootBottom || drop == DropLocation::rootLeft || drop == DropLocation::rootRight;
}


const bool DockManagerData::isViewDropLocation(DropLocation drop) const
{
    return drop == DropLocation::viewTop || drop == DropLocation::viewBottom || drop == DropLocation::viewLeft || drop == DropLocation::viewRight;
}


juce::ValueTree DockManagerData::getParentForDropType(const juce::ValueTree& tree, DockTypes type) const
{
    auto parent = searchParentsFor(tree, dockProps::dockType, (int)type);
    if (parent.isValid()) {return parent;}
    return searchParentsFor(tree, dockProps::dockType, (int)DockTypes::none);
}


const DockTypes DockManagerData::getTypeForLocation(DropLocation location) const
{
    return location == DropLocation::viewTop || location == DropLocation::viewBottom || location == DropLocation::parentTop || location == DropLocation::parentBottom  || location == DropLocation::rootTop || location == DropLocation::rootBottom ? DockTypes::vertical
            : location == DropLocation::viewLeft || location == DropLocation::viewRight || location == DropLocation::parentLeft || location == DropLocation::parentRight || location == DropLocation::rootRight || location == DropLocation::rootLeft ? DockTypes::horizontal
            : location == DropLocation::tabs ? DockTypes::tabs
            : DockTypes::none;
}


const int DockManagerData::getIndexForLocation(DropLocation location) const
{
    return location == DropLocation::viewTop || location == DropLocation::parentTop || location == DropLocation::viewLeft || location == DropLocation::parentLeft || location == DropLocation::rootLeft || location == DropLocation::rootTop ? 0 : -1;
}





/**
 ===================================
 MARK: - Setters -
 ===================================
 */

void DockManagerData::setLayoutName(const juce::String& layoutName)
{
    setName(_rootTree, layoutName);
}


void DockManagerData::setWindowMinimized(const juce::String& windowId, bool minimized)
{
    auto window = findTree(windowId);
    if (!window.isValid() || !isWindow(window)) {return;}
    window.setProperty(dockProps::windowMinimized, minimized, nullptr);
}


void DockManagerData::setWindowMaximized(const juce::String& windowId, bool maximised)
{
    auto window = findTree(windowId);
    if (!window.isValid() || !isWindow(window)) {return;}
    window.setProperty(dockProps::windowMaximized, maximised, nullptr);
}


void DockManagerData::setWindowLocked(const juce::String& windowId, bool locked)
{
    auto window = findTree(windowId);
    if (!window.isValid() || !isWindow(window)) {return;}
    window.setProperty(dockProps::lockedProperty, locked, nullptr);
}


void DockManagerData::setBounds(const juce::String& uuid, const juce::Rectangle<float>& bounds)
{
    auto tree = findTree(uuid);
    if (!tree.isValid()) {return;}
    setBounds(tree, bounds);
}


void DockManagerData::setBounds(juce::ValueTree& tree, const juce::Rectangle<float>& bounds)
{
    setPosition(tree, bounds.getPosition());
    setSize(tree, {bounds.getWidth(), bounds.getHeight()});
}


void DockManagerData::setPosition(const juce::String& uuid, const juce::Point<float>& position)
{
    auto tree = findTree(uuid);
    if (!tree.isValid()) {return;}
    setPosition(tree, position);
}


void DockManagerData::setPosition(juce::ValueTree& tree, const juce::Point<float>& position)
{
    setX(tree, position.getX());
    setY(tree, position.getY());
}


void DockManagerData::setSize(const juce::String& uuid, const juce::Point<float>& size)
{
    auto tree = findTree(uuid);
    if (!tree.isValid()) {return;}
    setSize(tree, size);
}


void DockManagerData::setSize(juce::ValueTree& tree, const juce::Point<float>& size)
{
    setWidth(tree, size.getX());
    setHeight(tree, size.getY());
}


void DockManagerData::setX(juce::String& uuid, float x)
{
    auto tree = findTree(uuid);
    if (!tree.isValid()) {return;}
    setX(tree, x); 
}


void DockManagerData::setY(juce::String& uuid, float y)
{
    auto tree = findTree(uuid);
    if (!tree.isValid()) {return;}
    setY(tree, y);
}


void DockManagerData::setX(juce::ValueTree& tree, float x)
{
    tree.setProperty(dockProps::xProperty, x, nullptr);
}


void DockManagerData::setY(juce::ValueTree& tree, float y)
{
    tree.setProperty(dockProps::yProperty, y, nullptr);
}


void DockManagerData::setWidth(juce::String& uuid, float width)
{
    auto tree = findTree(uuid);
    if (!tree.isValid()) {return;}
    setWidth(tree, width);
}


void DockManagerData::setHeight(juce::String& uuid, float height)
{
    auto tree = findTree(uuid);
    if (!tree.isValid()) {return;}
    setHeight(tree, height);
}



void DockManagerData::setWidth(juce::ValueTree& tree, float width)
{
    tree.setProperty(dockProps::widthProperty, juce::jmax<float>(width, 5), nullptr);
}


void DockManagerData::setHeight(juce::ValueTree& tree, float height)
{
    tree.setProperty(dockProps::heightProperty, juce::jmax<float>(height, 5), nullptr);
}


void DockManagerData::setDockType(juce::String& uuid, DockTypes type)
{
    auto tree = findTree(uuid);
    if (!tree.isValid()) {return;}
    setDockType(tree, type);
}


void DockManagerData::setDockType(juce::ValueTree& tree, DockTypes type)
{
    tree.setProperty(dockProps::dockType, (int)type, nullptr);
}


void DockManagerData::setName(juce::String& uuid, const juce::String& name)
{
    auto tree = findTree(uuid);
    if (!tree.isValid()) {return;}
    setName(tree, name);
}


void DockManagerData::setName(juce::ValueTree& tree, const juce::String& name)
{
    tree.setProperty(dockProps::nameProperty, name, nullptr);
}


void DockManagerData::setSelected(juce::ValueTree& tree, const juce::String& uuid)
{
    tree.setProperty(dockProps::selectedProperty, uuid, nullptr);
}




/**
 ===================================
 MARK: - Checks -
 ===================================
 */

const bool DockManagerData::isWindow(const juce::ValueTree& tree) const
{
    return tree.isValid() && tree.getType().toString() == dockIds::windowIdentifier;
}


const bool DockManagerData::isView(const juce::ValueTree& tree) const
{
    return tree.isValid() && tree.getType().toString() == dockIds::viewIdentifier;
}


const bool DockManagerData::isRootTree(const juce::ValueTree& tree) const
{
    return tree.isValid() && tree.getType().toString() == dockIds::rootTreeIdentifier;
}


const bool DockManagerData::isRootTree(const juce::String& treeId) const
{
    return isRootTree(findTree(treeId));
}


/**
 ===================================
 MARK: - Find -
 ===================================
 */

juce::ValueTree DockManagerData::getRootView(const juce::ValueTree& tree) const
{
    return searchParentsFor<juce::String>(tree, dockProps::nameProperty, dockIds::rootTreeIdentifier);
}


juce::ValueTree DockManagerData::getRootView(const juce::String& treeId) const
{
    return getRootView(findTree(treeId));
}


juce::ValueTree DockManagerData::getRootTreeForWindow(const juce::String& forWindow) const 
{
    auto window = findTree(forWindow);
    if (!window.isValid() || window.getNumChildren() == 0) {return juce::ValueTree();}
    return window.getChild(0);
}


const juce::ValueTree DockManagerData::findWindow(const juce::ValueTree& forTree) const
{
    if (isWindow(forTree))
        return forTree;
    
    if (forTree.getParent().isValid())
        return findWindow(forTree.getParent());
    else
        return juce::ValueTree();
}


const std::pair<juce::String, int> DockManagerData::getTreeForDockLocation(const juce::String& treeId, DropLocation drop) const
{
    auto tree = findTree(treeId);
    auto type = getTypeForLocation(drop);
    
    /// If the tree is a window, return the root view for that trees window
    if (isWindow(tree)) {return {getUuid(getRootTreeForWindow(treeId)), 0};}
    
    /// If is root drop, just return the root view for that trees window
    if (isRootDropLocation(drop)) {return {getUuid(getRootView(tree)) ,0};}
    
    auto isParent = isParentDropLocation(drop);
    if (!isParent) {return {treeId, 0};};
    
    auto parent = getParentForDropType(tree.getParent(), type);
    if (!parent.isValid()) {return {"", 0};}
    auto parentId = getUuid(parent);
    int index = -1;
    for (auto child : parent)
    {
        if (findTree(dockProps::uuidProperty, treeId, child).isValid())
        {
            index = parent.indexOf(child);
            break;
        }
    }
    
    return {getUuid(parent), index};
}


const juce::ValueTree DockManagerData::findTree(const juce::String& withUuid) const
{
    return findTree(dockProps::uuidProperty, withUuid, _rootTree);
}


const juce::ValueTree DockManagerData::findTree(const juce::String& propId, const juce::String& value, const juce::ValueTree& treeToSearch) const
{
    if (!treeToSearch.isValid()) {return juce::ValueTree();}
    if (treeToSearch.hasProperty(propId) && treeToSearch.getProperty(propId).toString() == value)
        return treeToSearch;
    
    for (auto child : treeToSearch)
    {
        if (child.hasProperty(propId) && child.getProperty(propId).toString() == value)
            return child;
        
        auto tree = findTree(propId, value, child);
        if (tree.isValid()) {return tree;}
    }
    
    return juce::ValueTree();
}


const juce::ValueTree DockManagerData::findTree(const juce::ValueTree& treeToSearch, const std::function<bool(const juce::ValueTree& tree)>& checkFunc) const
{
    if (!treeToSearch.isValid()) {return juce::ValueTree();}
    if (checkFunc(treeToSearch)) {return treeToSearch;}
    
    for (const auto& child : treeToSearch)
    {
        auto tree = findTree(child, checkFunc);
        if (tree.isValid()) {return tree;}
    }
    
    return juce::ValueTree();
}



template <typename T>
const juce::ValueTree DockManagerData::searchParentsFor(const juce::ValueTree& tree,const juce::String& propId, T value) const
{
    if (!tree.isValid()) {return juce::ValueTree();}
    const T property = tree.getProperty(propId);
    if (tree.hasProperty(propId) && property == value)
        return tree;
    else
        return searchParentsFor(tree.getParent(), propId, value);
}




/**
 ===================================
 MARK: - Utility -
 ===================================
 */

void DockManagerData::removeChildFromParent(juce::ValueTree& tree)
{
    if (!tree.isValid()) {return;}
    auto parent = tree.getParent();
    if (!parent.isValid()) {return;}
    parent.removeChild(tree, nullptr);
    
    /// Check for Selected Tab
    if (getDockType(parent) == DockTypes::tabs
        && getSelectedId(parent) == getUuid(tree)
        && parent.getNumChildren() > 1)
        setSelected(parent, getUuid(parent.getChild(0)));
}





/**
 ===================================
 MARK: - Mock -
 ===================================
 */

juce::Array<juce::Rectangle<float>> DockManagerData::mockAllRects()
{
    checkAllBounds();
    juce::Array<juce::Rectangle<float>> array;
    addTreeToMock(_rootTree, array);
    return array;
}


void DockManagerData::addTreeToMock(const juce::ValueTree& tree, juce::Array<juce::Rectangle<float>>& rects)
{
    auto bounds = getBounds(tree.getProperty(dockProps::uuidProperty));
    
    if (!bounds.isEmpty())
        rects.add(bounds);
        
    for (auto child : tree)
        addTreeToMock(child, rects);
}


void DockManagerData::checkAllBounds()
{
    for (auto child : _rootTree)
        checkBounds(child);
}


void DockManagerData::checkBounds(juce::ValueTree& tree)
{
    auto bounds = getBounds(tree).withPosition(0, 0);
    auto type = getDockType(tree);
    auto numChildren = tree.getNumChildren();
    
    auto toRemove = numChildren == 0 ? 0
                        : type == DockTypes::vertical ? bounds.getHeight() / numChildren
                        : type == DockTypes::horizontal ? bounds.getWidth() / numChildren
                        : 0;
    
    for (auto child : tree)
    {
        setBounds(child, type == DockTypes::vertical ? bounds.removeFromTop(toRemove)
                            : type == DockTypes::horizontal ? bounds.removeFromLeft(toRemove)
                            : bounds);
        checkBounds(child);
    }
}



/**
 ===================================
 MARK: - Random Name -
 ===================================
 */

const juce::String DockManagerData::getRandomName() const
{
    return "";
//    auto file = juce::File("/Users/christopherbaine/Desktop/Canvas/DockableWindowsV2/Source/common_words_list.txt");
//
//    auto str = file.loadFileAsString();
//    str = str.removeCharacters("\n\"");
//    auto strArray = juce::StringArray::fromTokens(str, ",", "'");
//    if (strArray.isEmpty()) {return "Empty";}
//    auto index = juce::Random::getSystemRandom().nextInt(strArray.size());
//    return strArray[index].trim();
}
