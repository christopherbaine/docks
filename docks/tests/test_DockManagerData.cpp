
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
    
}






/**
 ===================================
 MARK: - Find -
 ===================================
 */

TEST_CASE("findTreeWithUuid")
{
    
}


TEST_CASE("findTree")
{
    
}







/**
 ===================================
 MARK: - Mock -
 ===================================
 */


