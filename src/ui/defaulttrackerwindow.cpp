#include "defaulttrackerwindow.h"
#include "../uilib/hbox.h"
#include "../core/assets.h"

namespace Ui {

DefaultTrackerWindow::DefaultTrackerWindow(const char* title, SDL_Surface* icon, const Position& pos, const Size& size)
    : TrackerWindow(title, icon, pos, size)
{
    auto hbox = new HBox(0,0,_size.width,32);
    hbox->setBackground({0,0,0,255});
    hbox->setPadding(2);
    hbox->setSpacing(2);
    hbox->setGrow(1,1);
    _menu = hbox;
    addChild(_menu);
    
    _btnLoad = new ImageButton(0,0,32-4,32-4, asset("load.png").c_str());
    _btnLoad->setDarkenGreyscale(false);
    hbox->addChild(_btnLoad);
    _btnLoad->onClick += { this, [this](void*, int x, int y, int button) {
        onMenuPressed.emit(this, MENU_LOAD, 0);
    }};
    
    _btnReload = new ImageButton(0,0,32-4,32-4, asset("reload.png").c_str());
    _btnReload->setVisible(false);
    hbox->addChild(_btnReload);
    _btnReload->onClick += { this, [this](void*, int x, int y, int button) {
        onMenuPressed.emit(this, MENU_RELOAD, 0);
    }};
    
    _btnImport = new ImageButton(0,0,32-4,32-4, asset("import.png").c_str());
    _btnImport->setVisible(false);
    hbox->addChild(_btnImport);
    _btnImport->onClick += { this, [this](void*, int x, int y, int button) {
        onMenuPressed.emit(this, MENU_LOAD_STATE, 0);
    }};

    _btnExport = new ImageButton(0,0,32-4,32-4, asset("export.png").c_str());
    _btnExport->setVisible(false);
    hbox->addChild(_btnExport);
    _btnExport->onClick += { this, [this](void*, int x, int y, int button) {
        onMenuPressed.emit(this, MENU_SAVE_STATE, 0);
    }};

#ifndef __EMSCRIPTEN__ // no multi-window support (yet)
    _btnBroadcast = new ImageButton(0,0,32-4,32-4, asset("broadcast.png").c_str());
    _btnBroadcast->setVisible(false);
    hbox->addChild(_btnBroadcast);
    _btnBroadcast->onClick += { this, [this](void*, int x, int y, int button) {
        onMenuPressed.emit(this, MENU_BROADCAST, 0);
    }};
#endif
    
    _btnPackSettings = new ImageButton(32-4,0,32-4,32-4, asset("settings.png").c_str());
    _btnPackSettings->setVisible(false);
    hbox->addChild(_btnPackSettings);
    _btnPackSettings->onClick += { this, [this](void*, int x, int y, int button) {
        onMenuPressed.emit(this, MENU_PACK_SETTINGS, 0);
    }};

    _hboxAutoTrackers = new HBox(0,0,0,32-4);
    _hboxAutoTrackers->setPadding(4);
    _hboxAutoTrackers->setSpacing(6);
    hbox->addChild(_hboxAutoTrackers);

    _lblTooltip = new Label(0,0,0,0,_font,"");
    _lblTooltip->setHeight(32-4);
    _lblTooltip->setGrow(1,1);
    _lblTooltip->setTextAlignment(Label::HAlign::RIGHT, Label::VAlign::MIDDLE);
    hbox->addChild(_lblTooltip);
    
    _loadPackWidget = new LoadPackWidget(0,0,0,0,_fontStore);
    _loadPackWidget->setPosition({0,0});
    _loadPackWidget->setSize(_size);
    _loadPackWidget->setBackground({0,0,0});
    _loadPackWidget->setVisible(false);
    addChild(_loadPackWidget);
    
    _loadPackWidget->onPackSelected += {this,[this](void *s, const std::string& pack, const std::string& variant) {
        onPackSelected.emit(this,pack,variant);
    }};
    
    for (const auto& pair : {
            std::pair{_btnLoad, "Load Pack"},
            std::pair{_btnReload, "Reload Pack"},
            std::pair{_btnImport, "Import State"},
            std::pair{_btnExport, "Export State"},
            std::pair{_btnBroadcast, "Open Broadcast View"},
            std::pair{_btnPackSettings, "Open Pack Settings"}
    }) {
        if (!pair.first) continue;
        pair.first->setDarkenGreyscale(false);
        pair.first->setEnabled(false); // make greyscale
        const char* text = pair.second; // pointer to program memory
        pair.first->onMouseEnter += {this, [this,text](void *s, int x, int y, unsigned buttons)
        {
            ImageButton* btn = (ImageButton*)s;
            btn->setEnabled(true); // disable greyscale
            _lblTooltip->setText(text);
        }};
        pair.first->onMouseLeave += {this, [this,text](void *s)
        {
            ImageButton* btn = (ImageButton*)s;
            btn->setEnabled(false); // enable greyscale
            _lblTooltip->setText("");
        }};
    }
}

DefaultTrackerWindow::~DefaultTrackerWindow()
{
    // TODO: make sure widgets are deleted in container's destructor or delete here
    _btnLoad = nullptr;
    _btnReload = nullptr;
    _btnBroadcast = nullptr;
    _btnPackSettings = nullptr;
    _hboxAutoTrackers = nullptr;
    _lblsAutoTrackers.clear();
    _vboxProgress = nullptr;
    _lblProgressTitle = nullptr;
    _lblProgressPercent = nullptr;
    _lblProgressValues = nullptr;
    _pgbProgress = nullptr;
}

void DefaultTrackerWindow::setTracker(Tracker* tracker)
{
    std::string layout = _aspectRatio > 1 ? "tracker_horizontal" : _aspectRatio < 1 ? "tracker_vertical" : "tracker_default";
    if (tracker && !tracker->hasLayout(layout)) {
        if (tracker->hasLayout("tracker_default")) layout = "tracker_default";
        else if (tracker->hasLayout("tracker_horizontal")) layout = "tracker_horizontal";
        else if (tracker->hasLayout("tracker_vertical")) layout = "tracker_vertical";
    }
    setTracker(tracker, layout);
}

void DefaultTrackerWindow::setTracker(Tracker* tracker, const std::string& layout)
{
    if (_view && _view->getTracker() == tracker && _view->getLayoutRoot() == layout) return;

    TrackerWindow::setTracker(tracker, layout);
    if (tracker) {
        tracker->onLayoutChanged -= this;
        tracker->onLayoutChanged += {this, [this,tracker](void *s, const std::string& layout) {
            if (_btnBroadcast) _btnBroadcast->setVisible(tracker->hasLayout("tracker_broadcast"));
            if (_btnPackSettings) _btnPackSettings->setVisible(tracker->hasLayout("settings_popup"));
            setTracker(tracker); // reevaluate preferred and fall-back layout
        }};
        _view->onItemHover += {this, [this, tracker](void *s, const std::string& itemid) {
            if (!_lastHoverItem.empty()) {
                auto& item = tracker->getItemById(itemid);
                item.onChange -= this;
            }
            if (itemid.empty()) _lblTooltip->setText("");
            else {
                auto& item = tracker->getItemById(itemid);
                _lblTooltip->setText(item.getCurrentName());
                item.onChange += {this, [this, tracker, itemid](void* sender) {
                    const auto& item = tracker->getItemById(itemid);
                    _lblTooltip->setText(item.getCurrentName());
                }};
            }
            _lastHoverItem = itemid;
        }};
        if (_btnReload) _btnReload->setVisible(true);
        if (_btnImport) _btnImport->setVisible(true);
        if (_btnExport) _btnExport->setVisible(true);
    } else {
        if (_btnBroadcast) _btnBroadcast->setVisible(false);
        if (_btnPackSettings) _btnPackSettings->setVisible(false);
        if (_btnReload) _btnReload->setVisible(false);
        if (_btnImport) _btnImport->setVisible(false);
        if (_btnExport) _btnExport->setVisible(false);
    }
    raiseChild(_loadPackWidget);
}

void DefaultTrackerWindow::render(Renderer renderer, int offX, int offY)
{
    if (_view) {
        float oldAspectRatio = _aspectRatio;
        _aspectRatio = (float)getWidth() / (float)getHeight();
        Tracker* tracker = _view->getTracker();
        if (tracker && ((_aspectRatio > 1 && oldAspectRatio <= 1) || (_aspectRatio < 1 && oldAspectRatio >= 1)))
            setTracker(tracker); // reevaluate preferred and fall-back layout
    }
    TrackerWindow::render(renderer, offX, offY);
}

void DefaultTrackerWindow::setAutoTrackerState(int index, AutoTracker::State state, const std::string& name, const std::string& subname)
{
    if (index<0) {
        // clear all labels
        for (size_t i=0; i<_lblsAutoTrackers.size(); i++) {
            _autoTrackerStates[i] = state;
            auto lbl = _lblsAutoTrackers[i];
            lbl->setText(name);
            lbl->setWidth(lbl->getAutoWidth());
        }
        if (!_lblsAutoTrackers.empty()) {
            _hboxAutoTrackers->relayout();
            auto menu = dynamic_cast<HBox*>(_menu);
            if (menu) menu->relayout();
        }
        return;
    }

    Label* lbl = nullptr;
    if (_lblsAutoTrackers.size() > (size_t)index) {
        lbl = _lblsAutoTrackers[index];
        auto oldWidth = lbl->getWidth();
        lbl->setText(name);
        lbl->setWidth(lbl->getAutoWidth());
        if (lbl->getWidth() != oldWidth) {
            _hboxAutoTrackers->relayout();
            auto menu = dynamic_cast<HBox*>(_menu);
            if (menu) menu->relayout();
        }
        _autoTrackerStates[index] = state;
        _autoTrackerNames[index] = name;
        _autoTrackerSubNames[index] = subname;
    } else {
        printf("adding label #%d \"%s\"\n", index, name.c_str());
        lbl = new Label(0,0,0,32-4,_font, name);
        lbl->setWidth(lbl->getAutoWidth());
        lbl->setTextColor({0,0,0});
        lbl->onClick += {this, [this,index](void*, int x, int y, int button) {
            if (button == BUTTON_RIGHT) {
                onMenuPressed.emit(this, MENU_CYCLE_AUTOTRACKER, index);
            } else {
                onMenuPressed.emit(this, MENU_TOGGLE_AUTOTRACKER, index);
            }
        }};
        lbl->onMouseEnter += {this, [this,index](void *s, int x, int y, unsigned buttons)
        {
            auto state = _autoTrackerStates[index];
            const char* text = state == AutoTracker::State::Disconnected ? "Offline" :
                               state == AutoTracker::State::BridgeConnected ? "No Game/Console" :
                               state == AutoTracker::State::ConsoleConnected ? "Online" :
                               state == AutoTracker::State::Disabled ? "Disabled" :
                               state == AutoTracker::State::Unavailable ? nullptr : "Unknown";
            std::string name = _autoTrackerNames[index];
            if (name.empty()) name = "Auto Tracker";
            if (state == AutoTracker::State::ConsoleConnected && text) {
                if (!_autoTrackerSubNames[index].empty())
                    name = _autoTrackerSubNames[index];
            }
            if (text) {
                _lblTooltip->setText(name + ": " + text);
            }
        }};
        lbl->onMouseLeave += {this, [this](void *s)
        {
            _lblTooltip->setText("");
        }};
        _lblsAutoTrackers.push_back(lbl);
        _autoTrackerStates.push_back(state);
        _autoTrackerNames.push_back(name);
        _autoTrackerSubNames.push_back(subname);
        _hboxAutoTrackers->addChild(lbl);
        _hboxAutoTrackers->relayout();
        auto menu = dynamic_cast<HBox*>(_menu);
        if (menu) menu->relayout();
    }

    lbl->setTextAlignment(Label::HAlign::CENTER, Label::VAlign::MIDDLE);
    if (state == AutoTracker::State::Disconnected) {
        lbl->setTextColor({255,0,0});
    } else if (state == AutoTracker::State::BridgeConnected) {
        lbl->setTextColor({255,255,0});
    } else if (state == AutoTracker::State::ConsoleConnected) {
        lbl->setTextColor({0,255,0});
    } else if (state == AutoTracker::State::Disabled) {
        lbl->setTextColor({128,128,128});
    } else if (state == AutoTracker::State::Unavailable) {
        lbl->setTextColor({0,0,0}); // hidden
    }

    if (isHover(lbl)) {
        // update tool tip if current tool tip is AT state
        lbl->onMouseEnter.emit(lbl, 0, 0, 0);
    }
}

void DefaultTrackerWindow::setSize(Size size)
{
    TrackerWindow::setSize(size);
    _loadPackWidget->setSize(size);
    if (_vboxProgress) {
        _vboxProgress->setPosition({
            (_size.width - _vboxProgress->getWidth())/2,
            (_size.height - _vboxProgress->getHeight())/2
        });
    }
}

void DefaultTrackerWindow::showOpen()
{
    _loadPackWidget->update();
    _loadPackWidget->setVisible(true);
}
void DefaultTrackerWindow::hideOpen()
{
    _loadPackWidget->setVisible(false);
}

void DefaultTrackerWindow::showProgress(const std::string& text, int progress, int max)
{
    char percentText[5];
    if (progress>=0 && max>0) snprintf(percentText, 5, "%3u%%", 100U*progress/max);
    else percentText[0] = 0;
    std::string valuesText = format_bytes(progress) + "B";
    if (max) valuesText += " / " + format_bytes(max) + "B";

    if (!_vboxProgress) {
        int w = 300;
        int h = 69; // FIXME: should be able to auto-size this
        int x = (_size.width-w)/2;
        _vboxProgress = new VBox(x, 0, w, h);
        _vboxProgress->setPadding(5);
        _vboxProgress->setSpacing(5);
        _vboxProgress->setBackground({64,64,64});
        _lblProgressTitle = new Label(0,0,0,0, _font, text);
        _lblProgressTitle->setSize(_lblProgressTitle->getAutoSize());
        _lblProgressTitle->setTextAlignment(Label::HAlign::CENTER, Label::VAlign::TOP);
        _vboxProgress->addChild(_lblProgressTitle);
        _pgbProgress = new ProgressBar(1,0,w-2,20, progress, max);
        _vboxProgress->addChild(_pgbProgress);
        _hboxProgressTexts = new HBox(0,0,w,0);
        _vboxProgress->addChild(_hboxProgressTexts);
        _lblProgressPercent = new Label(0,0,w/2-5,0, _font, percentText);
        _lblProgressPercent->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::TOP);
        _lblProgressPercent->setHeight(_lblProgressPercent->getAutoHeight());
        _hboxProgressTexts->addChild(_lblProgressPercent);
        _lblProgressValues = new Label(0,0,w/2-5,0, _font, valuesText);
        _lblProgressValues->setTextAlignment(Label::HAlign::RIGHT, Label::VAlign::TOP);
        _lblProgressValues->setHeight(_lblProgressValues->getAutoHeight());
        _hboxProgressTexts->setHeight(_lblProgressValues->getHeight());
        _hboxProgressTexts->addChild(_lblProgressValues);
        addChild(_vboxProgress);
        _vboxProgress->setHeight(h);
        _vboxProgress->setTop((_size.height-_vboxProgress->getHeight())/2);
    } else {
        _lblProgressTitle->setText(text);
        _pgbProgress->setProgress(progress);
        _pgbProgress->setMax(max);
        _lblProgressPercent->setText(percentText);
        _lblProgressValues->setText(valuesText);
        _vboxProgress->setVisible(true);
        raiseChild(_vboxProgress);
    }
}

void DefaultTrackerWindow::hideProgress()
{
    _vboxProgress->setVisible(false);
}

} // namespace

