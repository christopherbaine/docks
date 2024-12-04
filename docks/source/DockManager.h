#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "DockManagerData.h"


/// Views
class DockingWindow;
class DockingComponent;


/**
 -------------------------------------------------------------
 ===================================
 MARK: - Dock Manager  -
 ===================================
 -------------------------------------------------------------
 */

class DockManager : private juce::ValueTree::Listener
{
    friend class test_DockManager;
    friend class DockingWindow; 
    friend class DropComponent;
    friend class WindowComponent;
    friend class DockingComponent;
    friend class HeaderComponent;
    friend class TabComponent;
    
public:
    /**
     Usings
     */
    using ViewMap = juce::HashMap<juce::String, std::shared_ptr<juce::Component>>;

    /**
     Delegate
     Must override to use a DockManager.
     */
    class Delegate
    {
    public:
        virtual ~Delegate() {}
        /**
         List of available views for the Dock Manager to use.
         You can pass in more than this, but this is what appears on right click
         @returns juce::StringArray of available view names
         */
        virtual const juce::StringArray getAvailableViews() const = 0;
        
        /**
         Create A View
         This must be overriden to create the views which you would like to see.
         @param: nameOfViewToCreate: The name of the view. You should probably handle empty strings, as well as check that the string contains the correct name. We might in the future add addendums to the name for identifying purposes
         @returns shared_ptr of the Component to view. This may be nullptr (which will not show a view) and you can retain it if you'd like, its a shared_ptr, so if you don't keep it, we will clean it up on close of view. (I reccommend not keeping it)
         */
        virtual std::shared_ptr<juce::Component> createView(const juce::String& nameOfViewToCreate) = 0;

        /**
         Get Display Name For View
         Use this if you'd like a different name displayed for the view, ie, if the nameOfTheView contains an Identifier or something
         @param: current name idenifier of view
         @returns: displayed name of view
         */
        virtual const juce::String getDisplayNameForView(const juce::String& nameOfView) {return nameOfView;}

        /**
         Get Window Name
         @returns the name for the windows that are created.
         */
        virtual const juce::String getDefaultWindowName() const = 0;
        
        /**
         Menu
         For Apple, just return nullptr and in your MenuBarModel (wherever that is) set it up to be the main menu
         If you return anything besides nullptr, it will add it to the window (which you probably dont want on macs)
         @returns the Menu to use for the window. Defaults to NoOp
         */
        virtual std::shared_ptr<juce::MenuBarComponent> getMenuForWindow(const juce::String& /*windowName*/) {return nullptr;}
        
        /**
         Adds a Keyboard Listener to a window. Wont retain
         Adds a keyboard listener to a window
         @return the key listener to add to the window. Can be nullptr
         */
        virtual juce::KeyListener* getKeyListenerForWindow(const juce::String& /*windowName*/) {return nullptr;}
        
        /**
        Footer for window
        @return Footer component to add to the window. Defaults to NoOp.
         */
        virtual std::shared_ptr<juce::Component> getFooterForWindow(const juce::String& /*windowName*/) {return nullptr;}
        
        /**
         Layout Did Update
         */
        virtual void didUpdateLayouts() {}
    };
    
    
    DockManager(Delegate& delegate);
    ~DockManager() override;
    
    /**
     Open View in New window
     @param: viewname: Name of view to pass in
     */
    void openViewInNewWindow(const juce::String& viewName, juce::Rectangle<float> bounds);
    
    /**
     Opens a new view in specific view
     Since this is a view which can exist or not exist, the view also can be created on it's own in a specific location
     @param viewName: The name of the new view
     @param regex: this is the name of the view to search for, when found it will open the view in this as a tab (and select it). Able to insert a regular expression in case you want to search more dilligently.
     @param fallbackType: In case this view cannot be found, this is where to dock it otherwise (pass in None for new window)
     */
    void openViewAsNewTab(const juce::String& viewName, const juce::String& regex, DropLocation fallbackType);
    
    /**
     Show View
     This will show the view if it currently exists (only works on tabs atm)
     It will only find the first version of the view it encounters.
     @param viewName: Name of view to show.
     @returns if view is shows
     */
    bool showView(const juce::String& viewName);
    
    /**
     Reset Display Names for Component
     Searches the components parents, to find the correct header/tabs to reset
     @param Component to search for parent to reset
     */
    void resetDisplayName(const juce::Component* component);
    
    /**
     Resets all display names
     */
    void resetAllDisplayNames();
    
    /**
     Save Template Layout
     This will call the Save Dialog window and strip the xml of unneeded info
     Such as the window size (since windows might open on different screens with different sizes)
     */
    void saveTemplate();
    
    /**
     Open Template
     This is basically the same thing as Open File...just with a dialog box
     This wants to be an XML File 
     */
    void openTemplate();
    
    /**
     Save Layout
     Save a layout to a file
     */
    void saveLayout(const juce::File& fileToSave);
   
    /**
     Save Layout
     Save a layout to a file
     */
    void saveLayout(juce::OutputStream& outputStream);
    
    /**
     Open Layouts
     Should be a valid XML file which was saved via SaveLayout
     */
    void openLayout(const juce::File& fileToOpen);
    
    /**
     Open Layouts From Input Stream
     Should be a valid XML file which was saved via SaveLayout
     */
    void openLayout(juce::InputStream& inputStream);
    
    
    /**
     Returns the Current layout as a juce::ValueTree
     */
    const juce::ValueTree getCurrentLayout() const;
    
    /**
     Add Overlay With Text
     This will add a grey semi-transparent overlay to all the windows (Ie for connection issues)
     @param show: show / hide the overlay
     @param textToShow: the text in the middle of the overlay
     */
    void showOverlayWithText(bool show, const juce::String& textToShow);
    
    /**
     Creates a 2 Column Window Layout
     @param windowName: Name for the new window
     @param views: The views to use for the layout. Should contain at least two views,
     */
    void create2Up(const juce::String& windowName, const juce::StringArray& views);
    
    /**
     Creates a 3 Column Window Layout
     @param windowName: Name for the new window
     @param views: The views to use for the layout. Should contain at least three views,
     */
    void create3Up(const juce::String& windowName, const juce::StringArray& views);
    
    /**
     Creates a 4 Column Window Layout
     @param windowName: Name for the new window
     @param views: The views to use for the layout. Should contain at least two views,
     */
    void create4Up(const juce::String& windowName, const juce::StringArray& views);
    
    /**
     Creates a 2 Column by 2 Row Window Layout
     @param windowName: Name for the new window
     @param views: The views to use for the layout. Should contain at least four views,
     */
    void create2By2(const juce::String& windowName, const juce::StringArray& views);
    
    /**
     Creates a 3 Column by 3 Row Window Layout
     @param windowName: Name for the new window
     @param views: The views to use for the layout. Should contain at least six views,
     */
    void create3By3(const juce::String& windowName, const juce::StringArray& views);
    
    /**
     Creates a 2 Row Window Layout
     @param windowName: Name for the new window
     @param views: The views to use for the layout. Should contain at least two views,
     */
    void create2Rows(const juce::String& windowName, const juce::StringArray& views);
    
    /**
     Creates a 3 Row Window Layout
     @param windowName: Name for the new window
     @param views: The views to use for the layout. Should contain at least three views,
     */
    void create3Rows(const juce::String& windowName, const juce::StringArray& views);

    /**
     Get All Components
     @returns a reference to the underlying view map, so you can access the views and manipulate them
     */
    const ViewMap& getAllComponents() const;

    /**
     Get Currently Focused Component
     */
    juce::Component* getCurrentlyFocusedComponent() const; 
    
    /**
     Get Component For Type
     returns the first for the type
     */
    template <typename T>
    const juce::Array<std::shared_ptr<T>> getComponentsForType() const
    {
        juce::Array<std::shared_ptr<T>> array {};
        for (auto comp : _components)
        {
            auto isT = std::dynamic_pointer_cast<T>(comp);
            if (!isT || comp->getPeer() == nullptr) {continue;}
            if (comp->getPeer()->isFocused() || comp->isKeyboardFocusContainer())
                array.insert(0, isT);
            else
                array.add(isT);
        }
        return array;
    }
private:
    
    /// Removal should run through the manager
    void removeView(const juce::String& treeId);
    
    /// Get Actual View
    std::shared_ptr<juce::Component> getComponent(const juce::String& withUuid, const juce::String& name);
    
    /// Popup Menus
    juce::PopupMenu getHeaderPopupMenu(const juce::ValueTree& tree);
    juce::PopupMenu getTabPopupMenu(const juce::ValueTree& tree);
    juce::PopupMenu getAddViewAtPopupMenu(const juce::ValueTree& tree);
    juce::PopupMenu getAddViewPopupMenu(const juce::ValueTree& tree, DropLocation location);
    void openInNewWindow(const juce::ValueTree& tree);
    
    /// Uitlity
    void printTree();
    
    /// ValueTree Listener
    void valueTreeChildAdded(juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    void valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    /// Drag and Drop Helpers
    void setCreateNewView(bool createNewView);
    void createNewWindow(const juce::String& withViewId, const juce::Point<float>& atPosition);
    
private:
 
    /// Delegate
    Delegate& _delegate;
    
    /// Window Tree
    DockManagerData _data;
    
    /// Windows
    using WindowMap = juce::HashMap<juce::String, std::shared_ptr<DockingWindow>>;
    WindowMap _windows;
    
    /// Components
    ViewMap _components;

    /// Drag and Drop Helper
    bool _createNewView = false;
    
    /// File Chooser
    std::unique_ptr<juce::FileChooser> _fileChooser;

    /// Menu For Mac
    std::shared_ptr<juce::MenuBarComponent> _menu;
    
    /// Throttler
    class UpdateThrottler;
    std::unique_ptr<UpdateThrottler> _throttler;
    
    /// Utility
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DockManager)
};


