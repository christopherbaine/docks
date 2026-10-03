
#include "catch2.hpp"
#include "../source/DockManager.h"
#include "../source/DockingWindow.h"
#include "../source/DockingComponent.h"
#include "../source/HeaderComponent.h"

/// Mock Delegate
class TestManagerDelegate : public DockManager::Delegate
{
public:
    const juce::StringArray getAvailableViews() const override { return {"Elements", "Canvas", "Cues", "Palette", "CueLists", "ElementLists", "Globals", "Monitors", "State"}; }
    std::shared_ptr<juce::Component> createView(const juce::String& /*nameOfViewToCreate*/) override { return nullptr; }
    const juce::String getDefaultWindowName() const override {return "Window";}
};


/// Delegate that creates a component for every named view, and remembers the last one made for each name
class ViewCreatingDelegate : public DockManager::Delegate
{
public:
    const juce::StringArray getAvailableViews() const override { return {"A", "B", "C", "D", "E", "F"}; }
    const juce::String getDefaultWindowName() const override {return "Window";}
    const juce::String getDisplayNameForView(const juce::String& nameOfView) override {return "Display " + nameOfView;}
    void didUpdateLayouts() override {numLayoutUpdates++;}
    
    std::shared_ptr<juce::Component> createView(const juce::String& nameOfViewToCreate) override
    {
        if (nameOfViewToCreate.isEmpty()) {return nullptr;}
        auto view = std::make_shared<juce::Component>();
        created.add(nameOfViewToCreate);
        views[nameOfViewToCreate] = view;
        return view;
    }
    
    bool isAlive(const juce::String& name) const
    {
        auto view = views.find(name);
        return view != views.end() && !view->second.expired();
    }
    
    juce::StringArray created;
    std::map<juce::String, std::weak_ptr<juce::Component>> views;
    int numLayoutUpdates = 0;
};


/// Describes a tree's children, e.g. "H[A V[B C]]". Containers show their type: H, V or T
static juce::String describe(const DockManagerData& data, const juce::ValueTree& tree)
{
    juce::StringArray parts;
    for (auto child : tree)
    {
        auto part = data.getName(child);
        if (child.getNumChildren() > 0)
        {
            auto type = data.getDockType(child);
            part += type == DockTypes::horizontal ? "H" : type == DockTypes::vertical ? "V" : type == DockTypes::tabs ? "T" : "";
            part += "[" + describe(data, child) + "]";
        }
        parts.add(part);
    }
    return parts.joinIntoString(" ");
}


static juce::ValueTree findByName(const juce::ValueTree& tree, const juce::String& name)
{
    for (auto child : tree)
    {
        if (child.getProperty(dockProps::nameProperty).toString() == name)
            return child;
        auto found = findByName(child, name);
        if (found.isValid())
            return found;
    }
    return {};
}


template <typename T>
static juce::Array<T*> findAll(juce::Component& parent)
{
    juce::Array<T*> found;
    for (auto* child : parent.getChildren())
    {
        if (auto* t = dynamic_cast<T*>(child))
            found.add(t);
        found.addArray(findAll<T>(*child));
    }
    return found;
}


static DockingComponent* findDockingComponent(juce::Component& parent, const juce::String& uuid)
{
    for (auto* comp : findAll<DockingComponent>(parent))
        if (comp->getUuid() == uuid)
            return comp;
    return nullptr;
}


static juce::StringArray itemNames(const juce::PopupMenu& menu)
{
    juce::StringArray names;
    for (juce::PopupMenu::MenuItemIterator it(menu); it.next();)
        if (!it.getItem().isSeparator)
            names.add(it.getItem().text);
    return names;
}


static const juce::PopupMenu::Item* findItem(const juce::PopupMenu& menu, const juce::StringArray& path)
{
    for (juce::PopupMenu::MenuItemIterator it(menu); it.next();)
    {
        const auto& item = it.getItem();
        if (item.text != path[0]) {continue;}
        if (path.size() == 1) {return &item;}
        if (item.subMenu == nullptr) {return nullptr;}
        auto rest = path;
        rest.remove(0);
        return findItem(*item.subMenu, rest);
    }
    return nullptr;
}


/// Runs the action of a menu item, following a path of sub menus
static bool invoke(const juce::PopupMenu& menu, const juce::StringArray& path)
{
    auto item = findItem(menu, path);
    if (item == nullptr || !item->action) {return false;}
    item->action();
    return true;
}


static juce::MouseEvent makeMouseEvent(juce::Component& component, juce::Point<float> position, bool wasDragged)
{
    auto now = juce::Time::getCurrentTime();
    return {juce::Desktop::getInstance().getMainMouseSource(), position, {},
            juce::MouseInputSource::defaultPressure, juce::MouseInputSource::defaultOrientation, juce::MouseInputSource::defaultRotation,
            juce::MouseInputSource::defaultTiltX, juce::MouseInputSource::defaultTiltY,
            &component, &component, now, position, now, 1, wasDragged};
}


/// Testing class with access to DockManager internals
class test_DockManager : public DockManager
{
public:
    test_DockManager(DockManager::Delegate& delegate) : DockManager(delegate) {}
    void printTree() {DockManager::printTree();}
    
    using DockManager::removeView;
    using DockManager::removeWindow;
    using DockManager::getComponent;
    using DockManager::getHeaderPopupMenu;
    using DockManager::getTabPopupMenu;
    using DockManager::getAddViewAtPopupMenu;
    using DockManager::getAddViewPopupMenu;
    using DockManager::setCreateNewView;
    using DockManager::createNewWindow;
    
    DockManagerData& data() {return _data;}
    const ViewMap& components() const {return _components;}
    int numWindows() const {return _windows.size();}
    
    juce::String windowId(int index) {return _data.getUuid(_data.getTree().getChild(index));}
    juce::String rootId(int index) {return _data.getUuid(_data.getTree().getChild(index).getChild(0));}
    DockingWindow* window(int index) {return _windows[windowId(index)].get();}
    juce::String layout(int index) {return describe(_data, _data.getTree().getChild(index).getChild(0));}
    juce::ValueTree tree(const juce::String& name) {return findByName(_data.getTree(), name);}
    juce::String idOf(const juce::String& name) {return _data.getUuid(tree(name));}
    std::shared_ptr<juce::Component> view(const juce::String& name)
    {
        auto id = idOf(name);
        return _components.contains(id) ? _components[id] : nullptr;
    }
    
    /// A single window whose root view holds a row of the given views, docked left to right
    void addRow(const juce::StringArray& views)
    {
        auto ids = _data.addNewWindow("w", {50, 50, 1000, 600});
        for (const auto& name : views)
            _data.dockNewView(ids.second, DropLocation::rootRight, name);
    }
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


TEST_CASE("dockManager_destructorReleasesViews")
{
    auto delegate = ViewCreatingDelegate();
    {
        auto manager = test_DockManager(delegate);
        manager.addRow({"A", "B"});
        REQUIRE(delegate.isAlive("A"));
    }
    CHECK_FALSE(delegate.isAlive("A"));
    CHECK_FALSE(delegate.isAlive("B"));
}


/**
 ===================================
 MARK: - Windows -
 ===================================
 */

TEST_CASE("addNewWindow")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    CHECK(manager.numWindows() == 0);
    
    manager.addRow({"A", "B"});
    REQUIRE(manager.numWindows() == 1);
    REQUIRE(manager.window(0) != nullptr);
    CHECK(manager.window(0)->isVisible());
    CHECK(manager.components().size() == 2);
    CHECK(delegate.created == juce::StringArray{"A", "B"});
}


TEST_CASE("removeWindow")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "B"});
    manager.addRow({"C"});
    REQUIRE(manager.numWindows() == 2);
    
    manager.removeWindow(manager.windowId(0));
    CHECK(manager.numWindows() == 1);
    CHECK(manager.components().size() == 1);
    CHECK_FALSE(delegate.isAlive("A"));
    CHECK_FALSE(delegate.isAlive("B"));
    CHECK(delegate.isAlive("C"));
}


TEST_CASE("closeAllWindows")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A"});
    manager.addRow({"B"});
    REQUIRE(manager.numWindows() == 2);
    
    manager.data().clearWindows();
    CHECK(manager.numWindows() == 0);
}


TEST_CASE("dockManager_closeButtonReleasesViews")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "B"});
    
    juce::DocumentWindow* window = manager.window(0);
    window->closeButtonPressed();
    CHECK(manager.numWindows() == 0);
    CHECK(manager.components().size() == 0);
    CHECK_FALSE(delegate.isAlive("A"));
    CHECK_FALSE(delegate.isAlive("B"));
}


/**
 ===================================
 MARK: - Presets -
 ===================================
 */

TEST_CASE("dockManager_presets")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    const juce::StringArray views {"A", "B", "C", "D", "E", "F"};
    
    SECTION("2Up")    {manager.create2Up("w", views);   CHECK(manager.layout(0) == "H[A B]");}
    SECTION("3Up")    {manager.create3Up("w", views);   CHECK(manager.layout(0) == "H[A B C]");}
    SECTION("4Up")    {manager.create4Up("w", views);   CHECK(manager.layout(0) == "H[A B C D]");}
    SECTION("2Rows")  {manager.create2Rows("w", views); CHECK(manager.layout(0) == "V[A B]");}
    SECTION("3Rows")  {manager.create3Rows("w", views); CHECK(manager.layout(0) == "V[A B C]");}
    SECTION("2By2")   {manager.create2By2("w", views);  CHECK(manager.layout(0) == "H[V[A B] V[C D]]");}
    SECTION("3By3")   {manager.create3By3("w", views);  CHECK(manager.layout(0) == "H[V[A B] V[C D] V[E F]]");}
    SECTION("too few views repeats the last")
    {
        manager.create3Up("w", {"A"});
        CHECK(manager.layout(0) == "H[A A A]");
    }
    SECTION("no views makes an empty window")
    {
        manager.create2Up("w", {});
        CHECK(manager.numWindows() == 1);
        CHECK(manager.layout(0).isEmpty());
    }
    
    CHECK(manager.numWindows() == 1);
    CHECK(manager.data().getName(manager.windowId(0)) == "w");
}


/**
 ===================================
 MARK: - Components -
 ===================================
 */

TEST_CASE("dockManager_getComponent")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    
    auto first = manager.getComponent("id1", "A");
    REQUIRE(first != nullptr);
    CHECK(manager.getComponent("id1", "A") == first);
    CHECK(delegate.created.size() == 1);
    
    CHECK(manager.getComponent("id2", "") == nullptr);
    CHECK_FALSE(manager.components().contains("id2"));
    
    auto nullDelegate = TestManagerDelegate();
    auto nullManager = test_DockManager(nullDelegate);
    nullManager.addRow({"A"});
    CHECK(nullManager.components().size() == 0);
}


TEST_CASE("dockManager_removeViewReleasesComponent")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "B"});
    auto viewB = manager.view("B");
    
    manager.removeView(manager.idOf("A"));
    CHECK(manager.layout(0) == "B");
    CHECK(manager.components().size() == 1);
    CHECK_FALSE(delegate.isAlive("A"));
    CHECK(manager.view("B") == viewB);
}


TEST_CASE("dockManager_getAllComponentsAndForType")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "B"});
    
    CHECK(manager.getAllComponents().size() == 2);
    CHECK(manager.getComponentsForType<juce::Component>().size() == 2);
    CHECK(manager.getComponentsForType<juce::Label>().isEmpty());
}


TEST_CASE("dockManager_getCurrentlyFocusedComponent")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "B"});
    
    auto viewB = manager.view("B");
    viewB->setWantsKeyboardFocus(true);
    viewB->grabKeyboardFocus();
    
    /// Focus depends on the window being active, which a test run can't guarantee
    if (viewB->hasKeyboardFocus(true))
        CHECK(manager.getCurrentlyFocusedComponent() == viewB.get());
    else
        CHECK(manager.getCurrentlyFocusedComponent() == nullptr);
}


/**
 ===================================
 MARK: - Menus -
 ===================================
 */

TEST_CASE("dockManager_headerMenu")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "B"});
    auto menu = manager.getHeaderPopupMenu(manager.tree("A"));
    
#if JUCE_DEBUG
    CHECK(itemNames(menu) == juce::StringArray{"Add Tab", "Add View", "Close View", "Open in New Window", "Lock Window", "Debug"});
    CHECK(itemNames(*findItem(menu, {"Debug"})->subMenu) == juce::StringArray{"Save As Template", "Open Template", "Test New Tree", "Print Tree"});
#else
    CHECK(itemNames(menu) == juce::StringArray{"Add Tab", "Add View", "Close View", "Open in New Window", "Lock Window"});
#endif
    CHECK(itemNames(*findItem(menu, {"Add Tab"})->subMenu) == delegate.getAvailableViews());
    CHECK(itemNames(*findItem(menu, {"Add View"})->subMenu) == delegate.getAvailableViews());
    CHECK(itemNames(*findItem(menu, {"Add View", "C"})->subMenu)
          == juce::StringArray{"View Left", "View Right", "View Top", "View Bottom",
                               "Window Left", "Window Right", "Window Top", "Window Bottom", "Tabs", "Window"});
    
    SECTION("Close View")
    {
        CHECK(invoke(menu, {"Close View"}));
        CHECK(manager.layout(0) == "B");
        CHECK_FALSE(delegate.isAlive("A"));
    }
    SECTION("Open in New Window")
    {
        CHECK(invoke(menu, {"Open in New Window"}));
        REQUIRE(manager.numWindows() == 2);
        CHECK(manager.layout(0) == "B");
        CHECK(manager.layout(1) == "A");
    }
    SECTION("Lock and Unlock Window")
    {
        CHECK(invoke(menu, {"Lock Window"}));
        CHECK(manager.data().isWindowLocked(manager.tree("A")));
        auto lockedMenu = manager.getHeaderPopupMenu(manager.tree("A"));
        CHECK(invoke(lockedMenu, {"Unlock Window"}));
        CHECK_FALSE(manager.data().isWindowLocked(manager.tree("A")));
    }
    SECTION("Add Tab")
    {
        CHECK(invoke(menu, {"Add Tab", "C"}));
        CHECK(manager.layout(0) == "H[T[A C] B]");
    }
    SECTION("Add View at location")
    {
        CHECK(invoke(menu, {"Add View", "C", "View Right"}));
        CHECK(manager.layout(0) == "H[A C B]");
    }
    SECTION("Add View in new window")
    {
        CHECK(invoke(menu, {"Add View", "C", "Window"}));
        REQUIRE(manager.numWindows() == 2);
        CHECK(manager.layout(1) == "C");
    }
#if JUCE_DEBUG
    SECTION("Print Tree")
    {
        CHECK(invoke(menu, {"Debug", "Print Tree"}));
    }
#endif
}


TEST_CASE("dockManager_tabMenu")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "D"});
    manager.data().dockNewView(manager.idOf("A"), DropLocation::tabs, "B");
    auto tabsId = manager.data().getUuid(manager.tree("A").getParent());
    manager.data().dockNewView(tabsId, DropLocation::tabs, "C");
    REQUIRE(manager.layout(0) == "H[T[A B C] D]");
    auto menu = manager.getTabPopupMenu(manager.tree("A"));
    
#if JUCE_DEBUG
    CHECK(itemNames(menu) == juce::StringArray{"Add Tab", "Add View", "Open In New Window", "Close Tab", "Close Other Tabs", "Debug"});
    CHECK(itemNames(*findItem(menu, {"Debug"})->subMenu) == juce::StringArray{"Save As Template", "Open Template", "Print Tree"});
#else
    CHECK(itemNames(menu) == juce::StringArray{"Add Tab", "Add View", "Open In New Window", "Close Tab", "Close Other Tabs"});
#endif
    
    SECTION("Close Tab")
    {
        CHECK(invoke(menu, {"Close Tab"}));
        CHECK(manager.layout(0) == "H[T[B C] D]");
        CHECK_FALSE(delegate.isAlive("A"));
    }
    SECTION("Close Other Tabs")
    {
        CHECK(invoke(menu, {"Close Other Tabs"}));
        CHECK(manager.layout(0) == "H[A D]");
        CHECK(delegate.isAlive("A"));
        CHECK_FALSE(delegate.isAlive("B"));
        CHECK_FALSE(delegate.isAlive("C"));
    }
    SECTION("Open In New Window")
    {
        CHECK(invoke(menu, {"Open In New Window"}));
        REQUIRE(manager.numWindows() == 2);
        CHECK(manager.layout(0) == "H[T[B C] D]");
        CHECK(manager.layout(1) == "A");
    }
}


/**
 ===================================
 MARK: - Drag Helpers -
 ===================================
 */

TEST_CASE("dockManager_createNewWindow")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "B"});
    
    manager.createNewWindow(manager.idOf("A"), {300, 200});
    CHECK(manager.numWindows() == 1);
    
    manager.setCreateNewView(true);
    manager.createNewWindow(manager.idOf("A"), {300, 200});
    REQUIRE(manager.numWindows() == 2);
    CHECK(manager.layout(1) == "A");
    
    /// The flag only allows one new window
    manager.createNewWindow(manager.idOf("B"), {300, 200});
    CHECK(manager.numWindows() == 2);
}


/**
 ===================================
 MARK: - Views -
 ===================================
 */

TEST_CASE("dockManager_viewWrappers")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    
    manager.openViewInNewWindow("A", {100, 100, 600, 400});
    REQUIRE(manager.numWindows() == 1);
    CHECK(manager.layout(0) == "A");
    
    manager.openViewAsNewTab("B", "^A$", DropLocation::rootRight);
    CHECK(manager.layout(0) == "T[A B]");
    
    CHECK(manager.showView("A"));
    CHECK(manager.data().isSelected(manager.tree("A")));
    CHECK_FALSE(manager.showView("missing"));
}


TEST_CASE("dockManager_overlayAndDisplayNames")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A"});
    auto content = manager.window(0)->getContentComponent();
    auto numChildren = content->getNumChildComponents();
    
    manager.showOverlayWithText(true, "Disconnected");
    CHECK(content->getNumChildComponents() == numChildren + 1);
    manager.showOverlayWithText(false, "");
    CHECK(content->getNumChildComponents() == numChildren);
    
    CHECK_NOTHROW(manager.resetAllDisplayNames());
    CHECK_NOTHROW(manager.resetDisplayName(manager.view("A").get()));
    CHECK_NOTHROW(manager.resetDisplayName(nullptr));
}


/**
 ===================================
 MARK: - Save / Open -
 ===================================
 */

TEST_CASE("dockManager_saveAndOpenLayout_stream")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "B"});
    manager.data().setWindowLocked(manager.windowId(0), true);
    
    juce::MemoryOutputStream out;
    manager.saveLayout(out);
    
    auto otherDelegate = ViewCreatingDelegate();
    auto other = test_DockManager(otherDelegate);
    other.addRow({"Old"});
    juce::MemoryInputStream in(out.getData(), out.getDataSize(), false);
    other.openLayout(in);
    
    REQUIRE(other.numWindows() == 1);
    CHECK(other.layout(0) == "H[A B]");
    CHECK(other.components().size() == 2);
    CHECK_FALSE(otherDelegate.isAlive("Old"));
    
    /// Locked windows stay locked when loaded
    CHECK(other.window(0)->isAlwaysOnTop());
    CHECK(findAll<juce::ImageButton>(*other.window(0)->getContentComponent()).size() == 1);
}


TEST_CASE("dockManager_saveAndOpenLayout_file")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "B"});
    
    juce::TemporaryFile temp(".xml");
    manager.saveLayout(temp.getFile());
    REQUIRE(temp.getFile().existsAsFile());
    
    auto otherDelegate = ViewCreatingDelegate();
    auto other = test_DockManager(otherDelegate);
    other.openLayout(temp.getFile());
    REQUIRE(other.numWindows() == 1);
    CHECK(other.layout(0) == "H[A B]");
}


TEST_CASE("dockManager_didUpdateLayouts_isThrottled")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    auto* messageManager = juce::MessageManager::getInstance();
    
    /// Many changes produce a single notification, 3 seconds after the last one
    manager.addRow({"A", "B"});
    CHECK(delegate.numLayoutUpdates == 0);
    messageManager->runDispatchLoopUntil(4000);
    CHECK(delegate.numLayoutUpdates == 1);
    
    manager.data().setLayoutName("Changed");
    messageManager->runDispatchLoopUntil(1000);
    CHECK(delegate.numLayoutUpdates == 1);
    messageManager->runDispatchLoopUntil(3000);
    CHECK(delegate.numLayoutUpdates == 2);
}


TEST_CASE("getTree")
{
    auto delegate = TestManagerDelegate();
    auto manager = test_DockManager(delegate);
    CHECK(manager.getCurrentLayout() == manager.data().getTree());
}


/**
 ===================================
 MARK: - Components on screen -
 ===================================
 */

TEST_CASE("dockManager_gui_rowAndColumnLayout")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    
    SECTION("row")
    {
        manager.create2Up("w", {"A", "B"});
        auto window = manager.window(0);
        window->setSize(1000, 600);
        auto a = window->getLocalArea(manager.view("A").get(), manager.view("A")->getLocalBounds());
        auto b = window->getLocalArea(manager.view("B").get(), manager.view("B")->getLocalBounds());
        CHECK(a.getRight() <= b.getX());
        CHECK(a.getY() == b.getY());
        CHECK(a.getHeight() == b.getHeight());
        CHECK(std::abs(a.getWidth() - b.getWidth()) <= 1);
    }
    SECTION("column")
    {
        manager.create2Rows("w", {"A", "B"});
        auto window = manager.window(0);
        window->setSize(1000, 600);
        auto a = window->getLocalArea(manager.view("A").get(), manager.view("A")->getLocalBounds());
        auto b = window->getLocalArea(manager.view("B").get(), manager.view("B")->getLocalBounds());
        CHECK(a.getBottom() <= b.getY());
        CHECK(a.getX() == b.getX());
        CHECK(a.getWidth() == b.getWidth());
        CHECK(std::abs(a.getHeight() - b.getHeight()) <= 1);
    }
}


TEST_CASE("dockManager_gui_dockingComponentQueries")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "C"});
    manager.data().dockNewView(manager.idOf("A"), DropLocation::tabs, "B");
    REQUIRE(manager.layout(0) == "H[T[A B] C]");
    
    auto content = manager.window(0)->getContentComponent();
    auto root = findDockingComponent(*content, manager.rootId(0));
    auto row = findDockingComponent(*content, manager.data().getUuid(manager.tree("C").getParent()));
    auto tabs = findDockingComponent(*content, manager.data().getUuid(manager.tree("A").getParent()));
    auto a = findDockingComponent(*content, manager.idOf("A"));
    auto c = findDockingComponent(*content, manager.idOf("C"));
    REQUIRE(root != nullptr);
    REQUIRE(row != nullptr);
    REQUIRE(tabs != nullptr);
    REQUIRE(a != nullptr);
    REQUIRE(c != nullptr);
    
    CHECK(root->hasSubItems());
    CHECK(tabs->isTabs());
    CHECK_FALSE(row->isTabs());
    CHECK(c->getName() == "C");
    CHECK_FALSE(c->hasSubItems());
    
    CHECK(tabs->shouldShowHeader());
    CHECK(c->shouldShowHeader());
    CHECK_FALSE(a->shouldShowHeader());
    CHECK_FALSE(row->shouldShowHeader());
    
    CHECK(root->getBoundsForSubview(manager.idOf("C"), -1) == c->localAreaToGlobal(c->getLocalBounds()).withTrimmedTop(25));
    CHECK(root->getBoundsForSubview("missing", -1).isEmpty());
    
    CHECK_NOTHROW(c->checkWillDisappear());
    CHECK_NOTHROW(c->layoutDidLoad());
    CHECK_NOTHROW(c->resetDisplayName());
}


TEST_CASE("dockManager_gui_tabs")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "C"});
    manager.data().dockNewView(manager.idOf("A"), DropLocation::tabs, "B");
    auto content = manager.window(0)->getContentComponent();
    
    /// The newest tab is selected and is the only one showing
    CHECK(manager.view("B")->isShowing());
    CHECK_FALSE(manager.view("A")->isShowing());
    
    auto tabs = findAll<TabComponent>(*content);
    REQUIRE(tabs.size() == 2);
    CHECK(tabs[0]->getDisplayName() == "Display A");
    CHECK(tabs[0]->getUuid() == manager.idOf("A"));
    CHECK(tabs[0]->getTree() == manager.tree("A"));
    CHECK_FALSE(tabs[0]->getSelected());
    CHECK(tabs[1]->getSelected());
    
    manager.showView("A");
    CHECK(manager.view("A")->isShowing());
    CHECK_FALSE(manager.view("B")->isShowing());
    
    /// Tab close button
    auto closeButtons = findAll<juce::ShapeButton>(*tabs[0]);
    REQUIRE(closeButtons.size() == 1);
    closeButtons[0]->onClick();
    CHECK(manager.layout(0) == "H[B C]");
    CHECK_FALSE(delegate.isAlive("A"));
}


TEST_CASE("dockManager_gui_lock")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A"});
    auto window = manager.window(0);
    auto content = window->getContentComponent();
    CHECK_FALSE(window->isAlwaysOnTop());
    CHECK(findAll<juce::ImageButton>(*content).isEmpty());
    
    manager.data().setWindowLocked(manager.windowId(0), true);
    CHECK(window->isAlwaysOnTop());
    auto buttons = findAll<juce::ImageButton>(*content);
    REQUIRE(buttons.size() == 1);
    
    /// The lock button unlocks
    buttons[0]->onClick();
    CHECK_FALSE(manager.data().isWindowLocked(manager.tree("A")));
    CHECK_FALSE(window->isAlwaysOnTop());
    CHECK(findAll<juce::ImageButton>(*content).isEmpty());
}


TEST_CASE("dockManager_gui_windowWritesBoundsAndState")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A"});
    auto window = manager.window(0);
    auto windowId = manager.windowId(0);
    auto windowTree = manager.data().getTree().getChild(0);
    
    window->setSize(700, 500);
    CHECK(manager.data().getSize(windowId) == juce::Point<float>(float(window->getWidth()), float(window->getHeight())));
    
    window->setTopLeftPosition(120, 140);
    CHECK(manager.data().getPosition(windowId) == window->getPosition().toFloat());
    
    juce::DocumentWindow* documentWindow = window;
    documentWindow->minimiseButtonPressed();
    CHECK(bool(windowTree.getProperty(dockProps::windowMinimized)));
    CHECK_FALSE(bool(windowTree.getProperty(dockProps::windowMaximized)));
    
    documentWindow->maximiseButtonPressed();
    CHECK_FALSE(bool(windowTree.getProperty(dockProps::windowMinimized)));
    CHECK(bool(windowTree.getProperty(dockProps::windowMaximized)));
}


TEST_CASE("dockManager_gui_resizeBelowMinimumRemovesView")
{
    auto delegate = ViewCreatingDelegate();
    auto manager = test_DockManager(delegate);
    manager.addRow({"A", "B"});
    manager.window(0)->setSize(1000, 600);
    
    auto content = manager.window(0)->getContentComponent();
    juce::Component* bar = nullptr;
    for (auto* comp : findAll<juce::Component>(*content))
        if (comp->getName() == "Resize")
            bar = comp;
    REQUIRE(bar != nullptr);
    
    /// Drag the bar left until A is narrower than the minimum size
    auto a = findDockingComponent(*content, manager.idOf("A"));
    REQUIRE(a != nullptr);
    auto start = bar->getMouseXYRelative().toFloat() + juce::Point<float>(float(a->getWidth() - 20), 0);
    bar->mouseDown(makeMouseEvent(*bar, start, false));
    bar->mouseDrag(makeMouseEvent(*bar, start, true));
    CHECK(a->getWidth() < 100);
    
    /// Releasing removes A, which destroys the bar and its parent
    bar->mouseUp(makeMouseEvent(*bar, start, true));
    CHECK(manager.layout(0) == "B");
    CHECK_FALSE(delegate.isAlive("A"));
}
