
#include "catch2.hpp"
#include "../source/DockManagerData.h"


class test_DockManagerData : public DockManagerData
{
public:
    /// Find
    juce::ValueTree findTree(const juce::String& propId, const juce::String& value) {return DockManagerData::findTree(propId, value, getTree());}
    juce::ValueTree findTree(const juce::String& uuid) {return DockManagerData::findTree(uuid);}
    template <typename T>
    const T getProperty(const juce::ValueTree& tree, const juce::String& propId) const {return DockManagerData::getProperty<T>(tree, propId);}
    juce::ValueTree getRootTreeForWindow(const juce::String& forWindow) const {return DockManagerData::getRootTreeForWindow(forWindow);}
    juce::ValueTree findTree(const juce::ValueTree& tree, const std::function<bool(const juce::ValueTree&)>& check) const {return DockManagerData::findTree(tree, check);}
    using DockManagerData::checkForOrphanedWindows;
    using DockManagerData::checkForOrphanedTreesIn;
    using DockManagerData::getRootView;
    using DockManagerData::findWindow;
    using DockManagerData::getIndexForLocation;
    
    juce::ValueTree findByName(const juce::String& name) {return findTree(dockProps::nameProperty, name);}
    juce::String idOf(const juce::String& name) {return getUuid(findByName(name));}
    juce::String layoutOfWindow(int index) {return layoutOf(getTree().getChild(index).getChild(0));}
    
    /// Describes a tree's children, e.g. "H[A V[B C]]". Containers show their type: H, V or T
    juce::String layoutOf(const juce::ValueTree& tree) const
    {
        juce::StringArray parts;
        for (auto child : tree)
        {
            auto part = getName(child);
            if (child.getNumChildren() > 0)
            {
                auto type = getDockType(child);
                part += type == DockTypes::horizontal ? "H" : type == DockTypes::vertical ? "V" : type == DockTypes::tabs ? "T" : "";
                part += "[" + layoutOf(child) + "]";
            }
            parts.add(part);
        }
        return parts.joinIntoString(" ");
    }
};


/// A window whose root view holds a row of the given views, docked left to right
static std::pair<juce::String, juce::String> addWindowWithRow(test_DockManagerData& data, const juce::StringArray& views)
{
    auto ids = data.addNewWindow("w");
    for (const auto& view : views)
        data.dockNewView(ids.second, DropLocation::rootRight, view);
    return ids;
}


static const std::array<DropLocation, 14> allDropLocations
{
    DropLocation::viewLeft, DropLocation::viewRight, DropLocation::viewTop, DropLocation::viewBottom,
    DropLocation::parentLeft, DropLocation::parentRight, DropLocation::parentTop, DropLocation::parentBottom,
    DropLocation::rootLeft, DropLocation::rootRight, DropLocation::rootTop, DropLocation::rootBottom,
    DropLocation::tabs, DropLocation::none
};


/**
 ===================================
 MARK: - Constructor -
 ===================================
 */
TEST_CASE("defaultConstructor")
{
    CHECK_NOTHROW(DockManagerData());
    CHECK(test_DockManagerData().getTree().isValid()); 
}





/**
 ===================================
 MARK: - Windows -
 ===================================
 */

TEST_CASE("dockManagerData_addWindow - default")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = data.addNewWindow("Chris");
    auto windowTree = data.findTree(windowId);
    CHECK(data.isWindow(windowTree));
    CHECK(data.getBounds(windowId) == juce::Rectangle<float>(10, 10, 1200, 800));
    CHECK(data.getName(windowId) == "Chris");
}


TEST_CASE("dockManagerData_addWindow_WithBounds")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = data.addNewWindow("Chris", {32, 43, 123, 534});
    auto windowTree = data.findTree(windowId);
    CHECK(windowId.isNotEmpty());
    CHECK(data.isWindow(windowTree));
    CHECK(data.getBounds(windowId) == juce::Rectangle<float>(32, 43, 123, 534));
    CHECK(data.getName(windowId) == "Chris");
}


TEST_CASE("dockManagerData_removeWindow", "[]")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = data.addNewWindow("Chris");
    CHECK(data.getTree().getNumChildren() > 0);
    data.removeWindow(windowId);
    CHECK(data.getTree().getNumChildren() == 0);
}


TEST_CASE("dockManagerData_clearWindows")
{
    auto data = test_DockManagerData();
    (void) data.addNewWindow("Chris1");
    (void) data.addNewWindow("Chris2");
    (void) data.addNewWindow("Chris3");
    CHECK(data.getTree().getNumChildren() == 3);
    data.clearWindows();
    CHECK(data.getTree().getNumChildren() == 0);
}





/**
 ===================================
 MARK: - Views -
 ===================================
 */

TEST_CASE("dockManagerData_AddView")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = data.addNewWindow("Test Window");
    REQUIRE(windowId.isNotEmpty());
    
    auto root = data.getRootTreeForWindow(windowId);
    auto viewId = data.addView(data.getUuid(root), "Test View");
    auto viewTree = data.findTree(viewId);
    
    CHECK(viewId.isNotEmpty());
    CHECK(viewTree.isValid());
    CHECK(root.getNumChildren() == 1);
    CHECK(data.isView(viewTree));
    CHECK(viewId == data.getProperty<juce::String>(viewTree, dockProps::uuidProperty));
    CHECK(data.getName(viewId) == "Test View");
    CHECK(viewTree.getParent() == root);
}


TEST_CASE("dockManagerData_RemoveView")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = data.addNewWindow("Test Window");
    auto root = data.getRootTreeForWindow(windowId);
    REQUIRE(windowId.isNotEmpty());
    
    auto viewId = data.addView(data.getUuid(root), "Test View");
    REQUIRE(viewId.isNotEmpty());
    CHECK(root.getNumChildren() == 1);
    
    data.removeView(viewId);
    CHECK(root.getNumChildren() == 0);
}


TEST_CASE("dockManagerData_dockView_inDifferentWindow")
{
    auto data = test_DockManagerData();
    auto [window1, rootId1] = data.addNewWindow("Window1");
    auto [window2, rootId2] = data.addNewWindow("Window2");
    auto rootTree1 = data.findTree(rootId1);
    auto rootTree2 = data.findTree(rootId2);
    
    REQUIRE(window1.isNotEmpty());
    REQUIRE(window2.isNotEmpty());
    
    auto view = data.addView(rootId1, "View1");
    CHECK(rootTree1.getNumChildren() == 1);
    CHECK(rootTree2.getNumChildren() == 0);
}


TEST_CASE("dockManagerData_dockView_toView_Tab")
{
    auto data = test_DockManagerData();
    auto [window1, rootId] = data.addNewWindow("Window1");
    auto root = data.getRootTreeForWindow(window1);
    REQUIRE(window1.isNotEmpty());
    
    auto view1 = data.addView(data.getUuid(root), "View1");
    REQUIRE(view1.isNotEmpty());

    auto view2 = data.addView(data.getUuid(root), "View2");
    REQUIRE(view2.isNotEmpty());
    CHECK(root.getNumChildren() == 2);
    
    auto subView = data.addView(view1, "SubView");
    REQUIRE(subView.isNotEmpty());
    CHECK(data.findTree(view1).getNumChildren() == 1);
    
}



TEST_CASE("dockManagerData_dockView_toView_Vertical")
{
    auto data = test_DockManagerData();
    auto [window1, rootId] = data.addNewWindow("Window1");
    auto root = data.getRootTreeForWindow(window1);
    REQUIRE(window1.isNotEmpty());
    
    auto view1 = data.addView(data.getUuid(root), "View1");
    REQUIRE(view1.isNotEmpty());

    auto view2 = data.addView(data.getUuid(root), "View2");
    REQUIRE(view2.isNotEmpty());
    CHECK(root.getNumChildren() == 2);
    
    auto subView = data.addView(view1, "SubView");
    REQUIRE(subView.isNotEmpty());
    CHECK(data.findTree(view1).getNumChildren() == 1);
}


TEST_CASE("dockManagerData_dockView_toView_Horizontal")
{
    auto data = test_DockManagerData();
    auto [window1, rootId] = data.addNewWindow("Window1");
    auto root = data.getRootTreeForWindow(window1);
    REQUIRE(window1.isNotEmpty());
    
    auto view1 = data.addView(data.getUuid(root), "View1");
    REQUIRE(view1.isNotEmpty());

    auto view2 = data.addView(data.getUuid(root), "View2");
    REQUIRE(view2.isNotEmpty());
    CHECK(root.getNumChildren() == 2);
    
    auto subView = data.addView(view1, "SubView");
    REQUIRE(subView.isNotEmpty());
    CHECK(data.findTree(view1).getNumChildren() == 1);
}


TEST_CASE("dockManagerData_dockView_TestingLots")
{
    auto data = test_DockManagerData();
    auto [window1, rootId] = data.addNewWindow("Window1");
    auto root = data.getRootTreeForWindow(window1);
    REQUIRE(window1.isNotEmpty());
    
    auto view1 = data.addView(data.getUuid(root), "View1");
    REQUIRE(view1.isNotEmpty());
    
    auto view2 = data.addView(data.getUuid(root), "View2");
    REQUIRE(view2.isNotEmpty());
    
    auto view3 = data.addView(data.getUuid(root), "View3");
    REQUIRE(view3.isNotEmpty());
    
    auto subView1 = data.addView(view1, "subView1");
    REQUIRE(subView1.isNotEmpty());
    
    auto subView2 = data.addView(view1, "subView2");
    REQUIRE(subView2.isNotEmpty());
    
    auto subsubView1 = data.addView(subView1, "subsubView1");
    REQUIRE(subsubView1.isNotEmpty());
}



/**
 ===================================
 MARK: - Checks -
 ===================================
 */

TEST_CASE("dockManagerData_isWindow")
{
    auto data = test_DockManagerData();
    juce::ValueTree tree = juce::ValueTree(dockIds::windowIdentifier);
    
    CHECK(data.isWindow(tree));
    CHECK(!data.isView(tree));
}


TEST_CASE("dockManagerData_isView")
{
    auto data = test_DockManagerData();
    juce::ValueTree tree = juce::ValueTree(dockIds::viewIdentifier);
    
    CHECK(!data.isWindow(tree));
    CHECK(data.isView(tree));
}


TEST_CASE("dockManagerData_canDock_inToTab")
{
//    auto data = test_DockManagerData();
//    auto window = data.addNewWindow("chris", {}, DockTypes::horizontal);
//    auto view1 = data.addView(window, "view1", DockTypes::horizontal);
//    auto view2 = data.addView(view1, "view2", DockTypes::tabs);
//    
//    CHECK_FALSE(data.canDock(view1, DropLocation::parentTop));
//    CHECK_FALSE(data.canDock(view1, DropLocation::parentBottom));
//    CHECK_FALSE(data.canDock(view1, DropLocation::parentLeft));
//    CHECK_FALSE(data.canDock(view1, DropLocation::parentRight));
//    CHECK_FALSE(data.canDock(view1, DropLocation::viewTop));
//    CHECK_FALSE(data.canDock(view1, DropLocation::viewBottom));
//    CHECK_FALSE(data.canDock(view1, DropLocation::viewLeft));
//    CHECK_FALSE(data.canDock(view1, DropLocation::viewRight));
//    CHECK(data.canDock(view1, DropLocation::tabs));
}


TEST_CASE("dockManagerData_canDock_parentIsTab")
{
//    auto data = test_DockManagerData();
//    auto window = data.addNewWindow("chris", {}, DockTypes::horizontal);
//    auto view1 = data.addView(window, "view1", DockTypes::horizontal);
//    auto view2 = data.addView(view1, "view2", DockTypes::horizontal);
//
//    CHECK_FALSE(data.canDock(view1, DropLocation::parentTop));
//    CHECK_FALSE(data.canDock(view1, DropLocation::parentBottom));
//    CHECK_FALSE(data.canDock(view1, DropLocation::parentLeft));
//    CHECK_FALSE(data.canDock(view1, DropLocation::parentRight));
//    CHECK_FALSE(data.canDock(view1, DropLocation::viewTop));
//    CHECK_FALSE(data.canDock(view1, DropLocation::viewBottom));
//    CHECK_FALSE(data.canDock(view1, DropLocation::viewLeft));
//    CHECK_FALSE(data.canDock(view1, DropLocation::viewRight));
//    CHECK_FALSE(data.canDock(view1, DropLocation::tabs));
}




/**
 ===================================
 MARK: - Getters -
 ===================================
 */

TEST_CASE("dockManagerData_getBounds")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = data.addNewWindow("w", {1, 2, 300, 400});
    auto window = data.findTree(windowId);
    
    CHECK(data.getBounds(window) == juce::Rectangle<float>(1, 2, 300, 400));
    CHECK(data.getPosition(windowId) == juce::Point<float>(1, 2));
    CHECK(data.getPosition(window) == juce::Point<float>(1, 2));
    CHECK(data.getSize(windowId) == juce::Point<float>(300, 400));
    CHECK(data.getSize(window) == juce::Point<float>(300, 400));
    CHECK(juce::exactlyEqual(data.getWidth(window), 300.0f));
    CHECK(juce::exactlyEqual(data.getHeight(window), 400.0f));
}


TEST_CASE("dockManagerData_getters_missingTree")
{
    auto data = test_DockManagerData();
    CHECK(data.getBounds("missing").isEmpty());
    CHECK(data.getPosition("missing") == juce::Point<float>());
    CHECK(data.getSize("missing") == juce::Point<float>());
    CHECK(data.getName("missing").isEmpty());
    CHECK(data.getDockType("missing") == DockTypes::none);
    CHECK(data.getUuid({}).isEmpty());
    CHECK(data.getSelectedId({}).isEmpty());
    CHECK_FALSE(data.isWindowLocked({}));
}


TEST_CASE("dockManagerData_dropLocationHelpers")
{
    auto data = test_DockManagerData();
    juce::StringArray names;
    int numParent = 0, numRoot = 0, numView = 0;
    
    for (auto location : allDropLocations)
    {
        auto isParent = data.isParentDropLocation(location);
        auto isRoot = data.isRootDropLocation(location);
        auto isView = data.isViewDropLocation(location);
        CHECK(int(isParent) + int(isRoot) + int(isView) <= 1);
        numParent += isParent ? 1 : 0;
        numRoot += isRoot ? 1 : 0;
        numView += isView ? 1 : 0;
        names.addIfNotAlreadyThere(data.dropLocationToString(location));
    }
    
    CHECK(numParent == 4);
    CHECK(numRoot == 4);
    CHECK(numView == 4);
    CHECK(names.size() == int(allDropLocations.size()));
    CHECK_FALSE(names.contains(""));
}


TEST_CASE("dockManagerData_getTypeAndIndexForLocation")
{
    auto data = test_DockManagerData();
    using DL = DropLocation;
    
    for (auto location : {DL::viewLeft, DL::viewRight, DL::parentLeft, DL::parentRight, DL::rootLeft, DL::rootRight})
        CHECK(data.getTypeForLocation(location) == DockTypes::horizontal);
    for (auto location : {DL::viewTop, DL::viewBottom, DL::parentTop, DL::parentBottom, DL::rootTop, DL::rootBottom})
        CHECK(data.getTypeForLocation(location) == DockTypes::vertical);
    CHECK(data.getTypeForLocation(DL::tabs) == DockTypes::tabs);
    CHECK(data.getTypeForLocation(DL::none) == DockTypes::none);
    
    for (auto location : allDropLocations)
    {
        auto first = location == DL::viewLeft || location == DL::viewTop || location == DL::parentLeft
                    || location == DL::parentTop || location == DL::rootLeft || location == DL::rootTop;
        CHECK(data.getIndexForLocation(location) == (first ? 0 : -1));
    }
}


TEST_CASE("dockManagerData_dockTypeToString")
{
    auto data = test_DockManagerData();
    CHECK(data.dockTypeToString(DockTypes::none) == "None");
    CHECK(data.dockTypeToString(DockTypes::tabs) == "Tabs");
    CHECK(data.dockTypeToString(DockTypes::vertical) == "Vertical");
    CHECK(data.dockTypeToString(DockTypes::horizontal) == "Horizontal");
}


TEST_CASE("dockManagerData_getTreeForDockLocation")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = addWindowWithRow(data, {"A", "B", "D"});
    data.dockNewView(data.idOf("B"), DropLocation::viewBottom, "C");
    REQUIRE(data.layoutOfWindow(0) == "H[A V[B C] D]");
    
    auto rowId = data.getUuid(data.findTree(rootId).getChild(0));
    auto columnId = data.getUuid(data.findByName("B").getParent());
    
    CHECK(data.getTreeForDockLocation(windowId, DropLocation::viewLeft) == std::pair<juce::String, int>(rootId, 0));
    CHECK(data.getTreeForDockLocation(data.idOf("A"), DropLocation::rootTop) == std::pair<juce::String, int>(rootId, 0));
    CHECK(data.getTreeForDockLocation(data.idOf("A"), DropLocation::viewRight) == std::pair<juce::String, int>(data.idOf("A"), 0));
    CHECK(data.getTreeForDockLocation(data.idOf("B"), DropLocation::parentLeft) == std::pair<juce::String, int>(rowId, 1));
    CHECK(data.getTreeForDockLocation(data.idOf("C"), DropLocation::parentTop) == std::pair<juce::String, int>(columnId, 1));
    CHECK(data.getTreeForDockLocation(data.idOf("A"), DropLocation::parentTop) == std::pair<juce::String, int>(rootId, 0));
    CHECK(data.getTreeForDockLocation("missing", DropLocation::parentTop) == std::pair<juce::String, int>("", 0));
}


TEST_CASE("dockManagerData_selection")
{
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A"});
    data.dockNewView(data.idOf("A"), DropLocation::tabs, "B");
    
    auto tabs = data.findByName("A").getParent();
    CHECK(data.getSelectedId(tabs) == data.idOf("B"));
    CHECK(data.isSelected(data.findByName("B")));
    CHECK_FALSE(data.isSelected(data.findByName("A")));
    
    data.setSelected(tabs, data.idOf("A"));
    CHECK(data.isSelected(data.findByName("A")));
}





/**
 ===================================
 MARK: - Setters -
 ===================================
 */

TEST_CASE("dockManagerData_setters_tree")
{
    auto data = test_DockManagerData();
    auto tree = data.getNewView("A");
    
    data.setBounds(tree, {1, 2, 30, 40});
    CHECK(data.getBounds(tree) == juce::Rectangle<float>(1, 2, 30, 40));
    data.setPosition(tree, {5, 6});
    CHECK(data.getPosition(tree) == juce::Point<float>(5, 6));
    data.setSize(tree, {70, 80});
    CHECK(data.getSize(tree) == juce::Point<float>(70, 80));
    data.setX(tree, 9);
    data.setY(tree, 10);
    CHECK(data.getPosition(tree) == juce::Point<float>(9, 10));
    data.setDockType(tree, DockTypes::vertical);
    CHECK(data.getDockType(tree) == DockTypes::vertical);
    data.setName(tree, "Renamed");
    CHECK(data.getName(tree) == "Renamed");
}


TEST_CASE("dockManagerData_setters_uuid")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = data.addNewWindow("w");
    auto id = data.addView(rootId, "A");
    
    data.setBounds(id, {1, 2, 30, 40});
    CHECK(data.getBounds(id) == juce::Rectangle<float>(1, 2, 30, 40));
    data.setPosition(id, {5, 6});
    CHECK(data.getPosition(id) == juce::Point<float>(5, 6));
    data.setSize(id, {70, 80});
    CHECK(data.getSize(id) == juce::Point<float>(70, 80));
    data.setX(id, 9);
    data.setY(id, 10);
    CHECK(data.getPosition(id) == juce::Point<float>(9, 10));
    data.setWidth(id, 11);
    data.setHeight(id, 12);
    CHECK(data.getSize(id) == juce::Point<float>(11, 12));
    data.setDockType(id, DockTypes::horizontal);
    CHECK(data.getDockType(id) == DockTypes::horizontal);
    data.setName(id, "Renamed");
    CHECK(data.getName(id) == "Renamed");
    
    juce::String missing = "missing";
    CHECK_NOTHROW(data.setX(missing, 1));
    CHECK_NOTHROW(data.setName(missing, "x"));
    CHECK_FALSE(data.findTree(dockProps::nameProperty, "x").isValid());
}


TEST_CASE("dockManagerData_setSize_clampsToMinimum")
{
    auto data = test_DockManagerData();
    auto tree = data.getNewView("A");
    data.setSize(tree, {1, -10});
    CHECK(data.getSize(tree) == juce::Point<float>(5, 5));
}


TEST_CASE("dockManagerData_windowState")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = addWindowWithRow(data, {"A"});
    auto window = data.findTree(windowId);
    
    CHECK_FALSE(data.isWindowLocked(data.findByName("A")));
    data.setWindowLocked(windowId, true);
    CHECK(data.isWindowLocked(data.findByName("A")));
    CHECK(data.isWindowLocked(window));
    
    data.setWindowMinimized(windowId, true);
    data.setWindowMaximized(windowId, true);
    CHECK(data.getProperty<bool>(window, dockProps::windowMinimized));
    CHECK(data.getProperty<bool>(window, dockProps::windowMaximized));
    
    /// Window setters ignore views
    auto viewId = data.idOf("A");
    data.setWindowLocked(viewId, false);
    data.setWindowMinimized(viewId, true);
    data.setWindowMaximized(viewId, true);
    CHECK(data.isWindowLocked(window));
    CHECK_FALSE(data.findByName("A").hasProperty(dockProps::windowMinimized));
    CHECK_FALSE(data.findByName("A").hasProperty(dockProps::windowMaximized));
    CHECK_FALSE(data.findByName("A").hasProperty(dockProps::lockedProperty));
}


TEST_CASE("dockManagerData_setLayoutName")
{
    auto data = test_DockManagerData();
    CHECK(data.getName(data.getTree()) == "Current Layout");
    data.setLayoutName("My Layout");
    CHECK(data.getName(data.getTree()) == "My Layout");
}





/**
 ===================================
 MARK: - Find -
 ===================================
 */

TEST_CASE("findTreeWithUuid")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = addWindowWithRow(data, {"A", "B"});
    auto idB = data.idOf("B");
    
    CHECK(data.findTree(windowId) == data.getTree().getChild(0));
    CHECK(data.findTree(rootId) == data.getRootTreeForWindow(windowId));
    CHECK(data.getName(data.findTree(idB)) == "B");
    CHECK_FALSE(data.findTree("missing").isValid());
    CHECK_FALSE(data.findTree("").isValid());
}


TEST_CASE("findTree")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = addWindowWithRow(data, {"A", "B"});
    
    CHECK(data.isView(data.findTree(dockProps::nameProperty, "B")));
    CHECK_FALSE(data.findTree(dockProps::nameProperty, "Z").isValid());
    
    auto row = data.findTree(data.getTree(), [&](const juce::ValueTree& tree) {return data.getDockType(tree) == DockTypes::horizontal;});
    CHECK(row == data.findByName("A").getParent());
    CHECK_FALSE(data.findTree(juce::ValueTree(), [](const juce::ValueTree&) {return true;}).isValid());
}


TEST_CASE("dockManagerData_rootViewAndWindowLookup")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = addWindowWithRow(data, {"A", "B"});
    auto a = data.findByName("A");
    
    CHECK(data.getRootView(a) == data.findTree(rootId));
    CHECK(data.getRootView(data.idOf("B")) == data.findTree(rootId));
    CHECK(data.findWindow(a) == data.findTree(windowId));
    CHECK_FALSE(data.findWindow(juce::ValueTree(dockIds::viewIdentifier)).isValid());
    CHECK(data.isRootTree(rootId));
    CHECK_FALSE(data.isRootTree(data.idOf("A")));
    CHECK_FALSE(data.getRootTreeForWindow("missing").isValid());
}





/**
 ===================================
 MARK: - Docking -
 ===================================
 */

TEST_CASE("dockManagerData_dockNewView_root")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = addWindowWithRow(data, {"A"});
    CHECK(data.layoutOfWindow(0) == "A");
    
    data.dockNewView(rootId, DropLocation::rootRight, "B");
    CHECK(data.layoutOfWindow(0) == "H[A B]");
    
    data.dockNewView(rootId, DropLocation::rootLeft, "C");
    CHECK(data.layoutOfWindow(0) == "H[C A B]");
    
    data.dockNewView(rootId, DropLocation::rootBottom, "D");
    CHECK(data.layoutOfWindow(0) == "V[H[C A B] D]");
    
    data.dockNewView(data.idOf("A"), DropLocation::rootTop, "E");
    CHECK(data.layoutOfWindow(0) == "V[E H[C A B] D]");
}


TEST_CASE("dockManagerData_dockNewView_view")
{
    auto data = test_DockManagerData();
    
    SECTION("left")
    {
        addWindowWithRow(data, {"A", "B"});
        data.dockNewView(data.idOf("A"), DropLocation::viewLeft, "C");
        CHECK(data.layoutOfWindow(0) == "H[C A B]");
    }
    SECTION("right")
    {
        addWindowWithRow(data, {"A", "B"});
        data.dockNewView(data.idOf("A"), DropLocation::viewRight, "C");
        CHECK(data.layoutOfWindow(0) == "H[A C B]");
    }
    SECTION("top")
    {
        addWindowWithRow(data, {"A", "B"});
        data.dockNewView(data.idOf("A"), DropLocation::viewTop, "C");
        CHECK(data.layoutOfWindow(0) == "H[V[C A] B]");
    }
    SECTION("bottom")
    {
        addWindowWithRow(data, {"A", "B"});
        data.dockNewView(data.idOf("A"), DropLocation::viewBottom, "C");
        CHECK(data.layoutOfWindow(0) == "H[V[A C] B]");
    }
    SECTION("tabs")
    {
        addWindowWithRow(data, {"A", "B"});
        auto newId = data.dockNewView(data.idOf("A"), DropLocation::tabs, "C");
        CHECK(data.layoutOfWindow(0) == "H[T[A C] B]");
        CHECK(data.getSelectedId(data.findByName("A").getParent()) == newId);
    }
    SECTION("tabs in tabs")
    {
        addWindowWithRow(data, {"A"});
        data.dockNewView(data.idOf("A"), DropLocation::tabs, "B");
        auto tabsId = data.getUuid(data.findByName("A").getParent());
        auto newId = data.dockNewView(tabsId, DropLocation::tabs, "C");
        CHECK(data.layoutOfWindow(0) == "T[A B C]");
        CHECK(data.getSelectedId(data.findTree(tabsId)) == newId);
    }
}


TEST_CASE("dockManagerData_dockView_tabIndex")
{
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A", "D"});
    data.dockNewView(data.idOf("A"), DropLocation::tabs, "B");
    auto tabsId = data.getUuid(data.findByName("A").getParent());
    data.dockNewView(tabsId, DropLocation::tabs, "C");
    REQUIRE(data.layoutOfWindow(0) == "H[T[A B C] D]");
    
    SECTION("reorder within tabs")
    {
        data.dockView(data.idOf("C"), tabsId, DropLocation::tabs, {}, 0);
        CHECK(data.layoutOfWindow(0) == "H[T[C A B] D]");
        CHECK(data.getSelectedId(data.findTree(tabsId)) == data.idOf("C"));
    }
    SECTION("insert from outside")
    {
        data.dockView(data.idOf("D"), tabsId, DropLocation::tabs, {}, 1);
        CHECK(data.layoutOfWindow(0) == "T[A D B C]");
    }
}


TEST_CASE("dockManagerData_dockView_parentRegressions")
{
    /// A row containing A, a column of B over C, then D
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A", "B", "D"});
    data.dockNewView(data.idOf("B"), DropLocation::viewBottom, "C");
    REQUIRE(data.layoutOfWindow(0) == "H[A V[B C] D]");
    
    auto drop = [&](const juce::String& viewToDock, const juce::String& over, DropLocation location)
    {
        auto [treeToDropAt, index] = data.getTreeForDockLocation(data.idOf(over), location);
        data.dockView(data.idOf(viewToDock), treeToDropAt, location, {}, index);
        return data.layoutOfWindow(0);
    };
    
    SECTION("parentLeft") {CHECK(drop("D", "B", DropLocation::parentLeft) == "H[A D V[B C]]");}
    SECTION("parentRight") {CHECK(drop("A", "B", DropLocation::parentRight) == "H[V[B C] A D]");}
    SECTION("parentTop") {CHECK(drop("D", "C", DropLocation::parentTop) == "H[A V[B D C]]");}
    SECTION("parentBottom") {CHECK(drop("A", "C", DropLocation::parentBottom) == "H[V[B C A] D]");}
}


TEST_CASE("dockManagerData_dockView_intoSelfOrDescendantIsIgnored")
{
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A", "B"});
    data.dockNewView(data.idOf("B"), DropLocation::viewBottom, "C");
    REQUIRE(data.layoutOfWindow(0) == "H[A V[B C]]");
    auto columnId = data.getUuid(data.findByName("B").getParent());
    
    data.dockView(data.idOf("A"), data.idOf("A"), DropLocation::viewLeft, {});
    CHECK(data.layoutOfWindow(0) == "H[A V[B C]]");
    
    data.dockView(columnId, data.idOf("C"), DropLocation::viewLeft, {});
    CHECK(data.layoutOfWindow(0) == "H[A V[B C]]");
    
    data.dockView("missing", data.idOf("C"), DropLocation::viewLeft, {});
    data.dockView(data.idOf("A"), "missing", DropLocation::viewLeft, {});
    CHECK(data.layoutOfWindow(0) == "H[A V[B C]]");
}


TEST_CASE("dockManagerData_dockView_noneOpensNewWindow")
{
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A", "B"});
    
    data.dockView(data.idOf("A"), data.idOf("B"), DropLocation::none, {500, 300});
    REQUIRE(data.getTree().getNumChildren() == 2);
    CHECK(data.layoutOfWindow(0) == "B");
    CHECK(data.layoutOfWindow(1) == "A");
    CHECK(data.getPosition(data.getTree().getChild(1)) == juce::Point<float>(500, 300));
}


TEST_CASE("dockManagerData_dockView_lastViewRemovesWindow")
{
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A"});
    addWindowWithRow(data, {"B"});
    REQUIRE(data.getTree().getNumChildren() == 2);
    
    data.dockView(data.idOf("A"), data.idOf("B"), DropLocation::viewRight, {});
    REQUIRE(data.getTree().getNumChildren() == 1);
    CHECK(data.layoutOfWindow(0) == "H[B A]");
}


TEST_CASE("dockManagerData_openInNewWindow")
{
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A", "B", "C"});
    
    data.openInNewWindow(data.idOf("A"), {100, 50}, {20, 30, 400, 300});
    REQUIRE(data.getTree().getNumChildren() == 2);
    CHECK(data.getBounds(data.getTree().getChild(1)) == juce::Rectangle<float>(20, 30, 400, 300));
    CHECK(data.layoutOfWindow(1) == "A");
    
    data.openInNewWindow(data.idOf("B"), {100, 50});
    REQUIRE(data.getTree().getNumChildren() == 3);
    CHECK(data.getPosition(data.getTree().getChild(2)) == juce::Point<float>(100, 50));
    CHECK(data.layoutOfWindow(0) == "C");
    
    data.openInNewWindow("missing", {});
    CHECK(data.getTree().getNumChildren() == 3);
}


TEST_CASE("dockManagerData_createInNewWindow")
{
    auto data = test_DockManagerData();
    data.createInNewWindow("Z", {1, 2, 300, 200});
    REQUIRE(data.getTree().getNumChildren() == 1);
    CHECK(data.layoutOfWindow(0) == "Z");
    CHECK(data.getBounds(data.getTree().getChild(0)) == juce::Rectangle<float>(1, 2, 300, 200));
}


TEST_CASE("dockManagerData_removeView_reselectsTab")
{
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A"});
    data.dockNewView(data.idOf("A"), DropLocation::tabs, "B");
    data.dockNewView(data.idOf("A"), DropLocation::tabs, "C");
    auto tabs = data.findByName("A").getParent();
    REQUIRE(data.getSelectedId(tabs) == data.idOf("C"));
    
    REQUIRE(data.layoutOfWindow(0) == "T[C A B]");
    
    data.removeView(data.idOf("C"));
    CHECK(data.layoutOfWindow(0) == "T[A B]");
    CHECK(data.getSelectedId(tabs) == data.idOf("A"));
    
    data.removeView(data.idOf("A"));
    CHECK(data.layoutOfWindow(0) == "B");
    
    data.removeView(data.idOf("B"));
    CHECK(data.getTree().getNumChildren() == 0);
}


TEST_CASE("dockManagerData_removeViewAndChildren")
{
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A", "C"});
    data.dockNewView(data.idOf("A"), DropLocation::tabs, "B");
    auto tabsId = data.getUuid(data.findByName("A").getParent());
    REQUIRE(data.layoutOfWindow(0) == "H[T[A B] C]");
    
    data.removeViewAndChildren(tabsId);
    CHECK(data.layoutOfWindow(0) == "C");
    CHECK_FALSE(data.findByName("A").isValid());
    CHECK_FALSE(data.findByName("B").isValid());
    
    data.removeViewAndChildren("missing");
    CHECK(data.layoutOfWindow(0) == "C");
}


TEST_CASE("dockManagerData_showView")
{
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A", "C"});
    data.dockNewView(data.idOf("A"), DropLocation::tabs, "B");
    auto tabs = data.findByName("A").getParent();
    REQUIRE(data.getSelectedId(tabs) == data.idOf("B"));
    
    CHECK(data.showView("A"));
    CHECK(data.getSelectedId(tabs) == data.idOf("A"));
    CHECK_FALSE(data.showView("C"));
    CHECK_FALSE(data.showView("missing"));
}


TEST_CASE("dockManagerData_openViewAsNewTab")
{
    auto data = test_DockManagerData();
    
    SECTION("no windows creates one")
    {
        data.openViewAsNewTab("N", "A", DropLocation::rootRight);
        CHECK(data.layoutOfWindow(0) == "N");
    }
    SECTION("matching view")
    {
        addWindowWithRow(data, {"A", "B"});
        data.openViewAsNewTab("N", "^A$", DropLocation::rootRight);
        CHECK(data.layoutOfWindow(0) == "H[T[A N] B]");
    }
    SECTION("fallback")
    {
        addWindowWithRow(data, {"A", "B"});
        data.openViewAsNewTab("N", "Z.*", DropLocation::rootRight);
        CHECK(data.layoutOfWindow(0) == "H[A B N]");
    }
    SECTION("invalid regex uses fallback")
    {
        addWindowWithRow(data, {"A", "B"});
        CHECK_NOTHROW(data.openViewAsNewTab("N", "(", DropLocation::rootLeft));
        CHECK(data.layoutOfWindow(0) == "H[N A B]");
    }
}


TEST_CASE("dockManagerData_addView_toWindowIsRejected")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = data.addNewWindow("w");
    CHECK(data.addView(windowId, "A").isEmpty());
    CHECK(data.findTree(rootId).getNumChildren() == 0);
}





/**
 ===================================
 MARK: - Orphans -
 ===================================
 */

TEST_CASE("dockManagerData_checkForOrphanedWindows_removesAdjacentEmptyWindows")
{
    auto data = test_DockManagerData();
    (void) data.addNewWindow("empty1");
    (void) data.addNewWindow("empty2");
    addWindowWithRow(data, {"A"});
    (void) data.addNewWindow("empty3");
    
    data.checkForOrphanedWindows();
    REQUIRE(data.getTree().getNumChildren() == 1);
    CHECK(data.layoutOfWindow(0) == "A");
}


TEST_CASE("dockManagerData_checkForOrphanedTrees_collapsesAdjacentContainers")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = data.addNewWindow("w");
    auto root = data.findTree(rootId);
    
    auto row = data.getNewView("", DockTypes::horizontal);
    root.addChild(row, -1, nullptr);
    
    auto column1 = data.getNewView("", DockTypes::vertical);
    data.setWidth(column1, 300);
    column1.addChild(data.getNewView("X", DockTypes::none), -1, nullptr);
    row.addChild(column1, -1, nullptr);
    
    auto column2 = data.getNewView("", DockTypes::vertical);
    column2.addChild(data.getNewView("Y", DockTypes::none), -1, nullptr);
    row.addChild(column2, -1, nullptr);
    
    row.addChild(data.getNewView("", DockTypes::tabs), -1, nullptr);
    row.addChild(data.getNewView("Z", DockTypes::none), -1, nullptr);
    REQUIRE(data.layoutOfWindow(0) == "H[V[X] V[Y]  Z]");
    
    data.checkForOrphanedTreesIn(data.getTree());
    CHECK(data.layoutOfWindow(0) == "H[X Y Z]");
    CHECK(juce::exactlyEqual(data.getWidth(data.findByName("X")), 300.0f));
}





/**
 ===================================
 MARK: - Save / Open -
 ===================================
 */

TEST_CASE("dockManagerData_saveAndOpenLayout_stream")
{
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A", "B"});
    data.dockNewView(data.idOf("A"), DropLocation::tabs, "C");
    
    juce::MemoryOutputStream out;
    REQUIRE(data.saveLayout(out));
    
    auto loaded = test_DockManagerData();
    addWindowWithRow(loaded, {"Old"});
    juce::MemoryInputStream in(out.getData(), out.getDataSize(), false);
    REQUIRE(loaded.openLayout(in));
    CHECK(loaded.getTree().isEquivalentTo(data.getTree()));
    CHECK(loaded.layoutOfWindow(0) == "H[T[A C] B]");
}


TEST_CASE("dockManagerData_openLayout_invalidKeepsTree")
{
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A"});
    auto before = data.getTree().createCopy();
    
    juce::MemoryInputStream in(juce::String("not xml").toRawUTF8(), 7, false);
    CHECK_FALSE(data.openLayout(in));
    CHECK(data.getTree().isEquivalentTo(before));
    
    CHECK_FALSE(data.openFromFile(juce::File()));
    CHECK(data.getTree().isEquivalentTo(before));
}


TEST_CASE("dockManagerData_saveAndOpen_file")
{
    auto data = test_DockManagerData();
    addWindowWithRow(data, {"A", "B"});
    
    juce::TemporaryFile temp(".xml");
    REQUIRE(data.saveToFile(temp.getFile()));
    
    auto loaded = test_DockManagerData();
    REQUIRE(loaded.openFromFile(temp.getFile()));
    CHECK(loaded.getTree().isEquivalentTo(data.getTree()));
}


TEST_CASE("dockManagerData_saveAsTemplate")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = addWindowWithRow(data, {"A", "B"});
    data.setWindowMinimized(windowId, true);
    data.setWindowMaximized(windowId, true);
    
    juce::TemporaryFile temp;
    auto saved = temp.getFile().withFileExtension("xml");
    REQUIRE(data.saveAsTemplate(temp.getFile()));
    CHECK(saved.existsAsFile());
    CHECK_FALSE(temp.getFile().existsAsFile());
    
    auto loaded = test_DockManagerData();
    REQUIRE(loaded.openFromFile(saved));
    saved.deleteFile();
    
    auto window = loaded.getTree().getChild(0);
    CHECK(loaded.getName(loaded.getTree()) == temp.getFile().getFileNameWithoutExtension());
    CHECK(loaded.layoutOfWindow(0) == "H[A B]");
    for (auto prop : {dockProps::xProperty, dockProps::yProperty, dockProps::widthProperty, dockProps::heightProperty,
                      dockProps::windowMinimized, dockProps::windowMaximized})
        CHECK_FALSE(window.hasProperty(prop));
    
    /// The original layout is unchanged
    CHECK(data.getBounds(windowId) == juce::Rectangle<float>(10, 10, 1200, 800));
}





/**
 ===================================
 MARK: - Mock -
 ===================================
 */

TEST_CASE("dockManagerData_mockAllRects")
{
    auto data = test_DockManagerData();
    auto [windowId, rootId] = data.addNewWindow("w", {0, 0, 1000, 500});
    data.dockNewView(rootId, DropLocation::rootRight, "A");
    data.dockNewView(rootId, DropLocation::rootRight, "B");
    
    auto rects = data.mockAllRects();
    CHECK(data.getBounds(data.idOf("A")) == juce::Rectangle<float>(0, 0, 500, 500));
    CHECK(data.getBounds(data.idOf("B")) == juce::Rectangle<float>(500, 0, 500, 500));
    CHECK(rects.contains({0, 0, 500, 500}));
    CHECK(rects.contains({500, 0, 500, 500}));
}

