#pragma once
#include <juce_gui_basics/juce_gui_basics.h>


constexpr bool PRINT_TREE_LISTENERS = false; 

/**
 -------------------------------------------------------------
 ===================================
 MARK: - Dock Identifiers -
 ===================================
 -------------------------------------------------------------
 */
namespace dockIds
{
    const juce::String rootTreeIdentifier = "root";
    const juce::String windowIdentifier = "window";
    const juce::String viewIdentifier = "view";
    const juce::String tabsIdentifier = "tabs";
}





/**
 -------------------------------------------------------------
 ===================================
 MARK: - Dock Property Identifiers -
 ===================================
 -------------------------------------------------------------
 */

namespace dockProps
{
    /// Property Identifiers
    const juce::String uuidProperty = "uuid";
    const juce::String xProperty = "x";
    const juce::String yProperty = "y";
    const juce::String widthProperty = "width";
    const juce::String heightProperty = "height";
    const juce::String nameProperty = "name";
    const juce::String selectedProperty = "selectedTab";
    const juce::String dockType = "dockType";
    const juce::String windowMinimized = "minimized";
    const juce::String windowMaximized = "maximized";
    const juce::String lockedProperty = "locked";

}


enum class DockTypes
{
    none = 0, tabs, vertical, horizontal
};


enum class DropLocation
{
    viewLeft, viewRight, viewTop, viewBottom,
    parentLeft, parentRight, parentTop, parentBottom,
    rootLeft, rootRight, rootTop, rootBottom,
    tabs, none
};




/**
 -------------------------------------------------------------
 ===================================
 MARK: - Dock Manager Data -
 ===================================
 -------------------------------------------------------------
 */
class DockManagerData
{
    friend class test_DockManagerData;
    friend class DockManager;
    
public:

    DockManagerData();
    ~DockManagerData();
    
    /** Save To File */
    bool saveAsTemplate(const juce::File& file);
    bool saveToFile(const juce::File& file);
    bool saveLayout(juce::OutputStream& outputStream);

    /** Open From file */
    bool openFromFile(const juce::File& file);
    bool openLayout(juce::InputStream& inputStream);

    
    /**
     Add New Window
     @param named: The name of the window
     @param bounds: the Bounds of the Window
     @return WindowUuid / RootViewUuid
     */
    const std::pair<juce::String, juce::String> addNewWindow(const juce::String& named, const juce::Rectangle<float> bounds = {10, 10, 1200, 800});
    
    /**
     Closes a window
     */
    void removeWindow(const juce::String& windowId);
    
    /**
     Clear All Windows
     */
    void clearWindows();
    
    
    /// Views
    juce::ValueTree getNewView(const juce::String viewName, DockTypes dockAt = DockTypes::tabs);
    const juce::String addView(const juce::String& toUuid, const juce::String& viewName, DockTypes dockAt = DockTypes::tabs);
    void removeView(const juce::String& viewId);
    void removeViewAndChildren(const juce::String& viewid); 
    bool showView(const juce::String& viewName); 

    /// Docking
    const bool canDock(const juce::String& viewToDockIn, DropLocation location) const;
    void openViewAsNewTab(const juce::String& viewName, const juce::String& regex, DropLocation fallbackType);
    const juce::String dockNewView(const juce::String& viewToDockIn, DropLocation location, const juce::String& name = "");
    void createInNewWindow(const juce::String& viewName, const juce::Rectangle<float>& bounds);
    void openInNewWindow(const juce::String& treeId, const juce::Point<float>& position, const juce::Rectangle<float>& bounds = {});
    void dockView(const juce::String& viewToDock, const juce::String& viewToDockIn, DropLocation location, juce::Point<float> dropPosition, int index = -1);
    bool dockView(const juce::ValueTree& viewToDock, const juce::ValueTree& viewToDockIn, DropLocation location, int index, juce::Point<float> dropPosition = {});
    const DockTypes getTypeForLocation(DropLocation location) const;
    
    /// Getters
    const juce::Rectangle<float> getBounds(const juce::String& uuid) const;
    const juce::Rectangle<float> getBounds(const juce::ValueTree& fromTree) const;
    const juce::Point<float> getPosition(const juce::String& uuid) const;
    const juce::Point<float> getPosition(const juce::ValueTree& fromTree) const;
    const juce::Point<float> getSize(const juce::String& uuid) const;
    const juce::Point<float> getSize(const juce::ValueTree& fromTree) const;
    const float getWidth(const juce::ValueTree& tree) const;
    const float getHeight(const juce::ValueTree& tree) const;
    const juce::String getName(const juce::String& uuid) const;
    const juce::String getName(const juce::ValueTree& fromTree) const;
    const DockTypes getDockType(const juce::String& uuid) const;
    const DockTypes getDockType(const juce::ValueTree& fromTree) const;
    const juce::String getUuid(const juce::ValueTree& tree) const;
    const juce::String getSelectedId(const juce::ValueTree& tree) const;
    const bool isSelected(const juce::ValueTree& tree) const;
    const bool isWindowLocked(const juce::ValueTree& tree) const;
    const juce::String dropLocationToString(DropLocation drop) const; 
    const juce::String dockTypeToString(DockTypes type) const; 
    const std::pair<juce::String, int> getTreeForDockLocation(const juce::String& treeId, DropLocation drop) const;
    const bool isParentDropLocation(DropLocation drop) const;
    const bool isRootDropLocation(DropLocation drop) const;
    const bool isViewDropLocation(DropLocation drop) const;

    /// Setters
    void setLayoutName(const juce::String& layoutName); 
    void setWindowMinimized(const juce::String& windowId, bool minimized);
    void setWindowMaximized(const juce::String& windowId, bool maximised);
    void setWindowLocked(const juce::String& windowId, bool locked);
    void setBounds(const juce::String& uuid, const juce::Rectangle<float>& bounds);
    void setBounds(juce::ValueTree& tree, const juce::Rectangle<float>& bounds);
    void setPosition(const juce::String& uuid, const juce::Point<float>& position);
    void setPosition(juce::ValueTree& tree, const juce::Point<float>& position);
    void setSize(const juce::String& uuid, const juce::Point<float>& size);
    void setSize(juce::ValueTree& tree, const juce::Point<float>& size);
    void setX(juce::String& uuid, float x);
    void setY(juce::String& uuid, float y);
    void setX(juce::ValueTree& tree, float x);
    void setY(juce::ValueTree& tree, float y);
    void setWidth(juce::String& uuid, float width);
    void setHeight(juce::String& uuid, float height);
    void setWidth(juce::ValueTree& tree, float width);
    void setHeight(juce::ValueTree& tree, float height);
    void setDockType(juce::String& uuid, DockTypes type);
    void setDockType(juce::ValueTree& tree, DockTypes type);
    void setName(juce::String& uuid, const juce::String& name);
    void setName(juce::ValueTree& tree, const juce::String& name);
    void setSelected(juce::ValueTree& tree, const juce::String& uuid);
    
    /// Checks
    const bool isWindow(const juce::ValueTree& tree) const;
    const bool isRootTree(const juce::ValueTree& tree) const;
    const bool isRootTree(const juce::String& treeId) const;
    const bool isView(const juce::ValueTree& tree) const;
    
    /// Utility
    juce::ValueTree getTree();
    void printTree();
    
    /// Mock
    juce::Array<juce::Rectangle<float>> mockAllRects();
    void checkAllBounds();
    void checkBounds(juce::ValueTree& tree);
    void addTreeToMock(const juce::ValueTree& tree, juce::Array<juce::Rectangle<float>>& rects);
    
protected:
    
    /// Docking Helpers
    bool dockInView(juce::ValueTree treeToDock, juce::ValueTree treeToDockIn, DropLocation typeToDock, int index);
    bool dockInParent(juce::ValueTree treeToDock, juce::ValueTree treeToDockIn, DropLocation typeToDock, int index);
    bool dockInRoot(juce::ValueTree treeToDock, juce::ValueTree treeToDockIn, DropLocation typeToDock, int index);
    bool dockInNewWindow(juce::ValueTree treeToDock, juce::Point<float> dropPosition, juce::Rectangle<float> windowBounds = {});
    const int getIndexForLocation(DropLocation location) const;
    
    /// Check For Orphans
    void checkForOrphanedTrees();
    void checkForOrphanedTreesIn(juce::ValueTree tree);
    void checkForOrphanedWindows();
    
    /// To Delete
    void checkBounds(const juce::String& forView);
    const juce::String getRandomName() const;
    
    /// Root View Helpers
    const juce::String addRootView(const juce::String& toWindow);
    juce::ValueTree getRootView(const juce::ValueTree& tree) const;
    juce::ValueTree getRootView(const juce::String& treeId) const;
    juce::ValueTree getRootTreeForWindow(const juce::String& forWindow) const;

    /// Find Tree
    const juce::ValueTree findTree(const juce::String& withUuid) const;
    const juce::ValueTree findTree(const juce::String& propId, const juce::String& value, const juce::ValueTree& treeToSearch) const;
    const juce::ValueTree findTree(const juce::ValueTree& treeToSearch, const std::function<bool(const juce::ValueTree& tree)>& checkFunc) const;
    const juce::ValueTree findWindow(const juce::ValueTree& forTree) const;
    void removeChildFromParent(juce::ValueTree& tree);
    juce::ValueTree getParentForDropType(const juce::ValueTree& tree, DockTypes type) const;
    juce::ValueTree addAndMoveTree(const juce::ValueTree& tree, DockTypes type);
    
    /// Search Parents for Value
    template <typename T>
    const juce::ValueTree searchParentsFor(const juce::ValueTree& tree,const juce::String& propId, T value) const;
    
    /// Get Properties from tree
    template <typename T>
    const T getProperty(const juce::ValueTree& tree, const juce::String& propId) const;
    
private:
    
    /// Tree Data
    juce::ValueTree _rootTree = juce::ValueTree(dockIds::rootTreeIdentifier);
    
    /// Utility
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DockManagerData)
};
