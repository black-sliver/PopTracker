#include "trackerwindow.h"
#include "../uilib/hbox.h"
#include "../uilib/imagebutton.h"
#include "../core/assets.h"

namespace Ui {

const std::string TrackerWindow::MENU_LOAD = "load";
const std::string TrackerWindow::MENU_RELOAD = "reload";
const std::string TrackerWindow::MENU_LOAD_STATE = "load-state";
const std::string TrackerWindow::MENU_SAVE_STATE = "save-state";
const std::string TrackerWindow::MENU_BROADCAST = "broadcast";
const std::string TrackerWindow::MENU_PACK_SETTINGS = "pack-settings";
const std::string TrackerWindow::MENU_TOGGLE_AUTOTRACKER = "toggle-autotracker";

TrackerWindow::TrackerWindow(const char* title, SDL_Surface* icon, const Position& pos, const Size& size)
    : Window(title, icon, pos, size)
{
    _baseTitle = title;
    
    printf("Loading small font ...\n");
    
    _menu = nullptr;
    _view = nullptr;
    _resizeScheduled = false;
}

TrackerWindow::~TrackerWindow()
{
    _menu = nullptr;
    _view = nullptr;
}

void TrackerWindow::setTracker(Tracker* tracker)
{
}

void TrackerWindow::setTracker(Tracker* tracker, const std::string& layout)
{
    removeChild(_view);
    delete _view;
    _view = nullptr;
    if (tracker) {
        _rendered = false;
        int left=0, top=_menu?(_menu->getTop()+_menu->getHeight()):0;
        //int w=_size.width-left, h=_size.height-top;
        int w=300, h=200; // the current layout hacks work better when growing than shrinking
        _view = new TrackerView(left, top, w, h, tracker, layout, _fontStore);
        _rendered = false;
        addChild(_view);
        
        // force-resize after the first render pass to force correct relayout.
        // TODO: fix this in TrackerView and Containers
        setSize(_size/*{_size.width-1, _size.height-1}*/);
        
        _view->onMinSizeChanged += {this, [this](void *s) {
            //Size curSize = Size::FromPosition(_view->getPosition()+_view->getSize());
            Size curSize = getSize();
            Size minSize = Size::FromPosition(_view->getPosition()+_view->getMinSize());
            Size newSize = curSize || minSize || Size{96,96};
            if (newSize != curSize) {
                printf("Layout changed ... ");
                printf("%d,%d || %d,%d || 96,96= %d,%d\n", curSize.width, curSize.height, minSize.width, minSize.height, newSize.width, newSize.height);
                resize(newSize);
            }
        }};
        // once the layout is fixed:
        //_view->onMinSizeChanged.emit(_view); // for initial window-resize
        auto pack = tracker->getPack();
        setTitle(_baseTitle + " - " + pack->getVariantTitle());
    } else {
        setTitle(_baseTitle);
    }
}

void TrackerWindow::setAutoTrackerState(AutoTracker::State state)
{
}


std::list< std::pair<std::string,std::string> > TrackerWindow::getHints() const
{
    if (_view) return _view->getHints();
    return {};
}

void TrackerWindow::setSize(Size size)
{
    printf("setSize(%d,%d)\n", size.width, size.height);
    //if (size == _size) return; // until requirement to render twice if fixed
    // don't resize multiple times per frame -> schedule resize instead
    _resizeSize = size;
    _resizeScheduled = true;
}

void TrackerWindow::render(Renderer renderer, int offX, int offY)
{
    // FIX THIS: we need to setSize twice to fully do all auto-sizing, see below
    if (_resizeScheduled) { // TODO: move this work-around to trackerView::render instead?
        printf("Resizing...\n");
        Window::setSize(_resizeSize);
        if (_menu) _menu->setSize({_size.width-_menu->getLeft(), _menu->getHeight()});
        if (_view && _rendered) {
            _view->setSize({_size.width-_view->getLeft(), _size.height-_view->getTop()});
        }
#if 0
        dumpTree();
#endif
        _resizeScheduled = false;
    }
    Window::render(renderer, offX, offY);
    if (!_rendered) {
        printf("First render(%d,%d)\n", _size.width, _size.height);
        _rendered = true;
        // force-resize after the first render pass to force correct relayout.
        // TODO: fix this in TrackerView
        setSize(_size/*{_size.width+1, _size.height+1}*/);
        Size minSize = {0,0};
        if (_view) {
            minSize = _view->getMinSize();
        }
        if (_menu) {
            minSize.height += _menu->getHeight();
        }
        setMinSize(minSize);
    }
    else if (_centerPos.left>=0 && _centerPos.top>=0) {
        setPosition({_centerPos.left-_size.width/2, _centerPos.top-_size.height/2});
        _centerPos = {-1,-1};
    }
}

} // namespace
