#ifndef _UI_LOADPACKWIDGET_H
#define _UI_LOADPACKWIDGET_H

#include "../uilib/container.h"
#include "../uilib/simplecontainer.h"
#include "../uilib/hbox.h"
#include "../uilib/vbox.h"
#include "../uilib/scrollvbox.h"
#include "../uilib/label.h"
#include "../uilib/button.h"
#include "../uilib/imagebutton.h"
#include "../uilib/textfield.h"
#include "../uilib/fontstore.h"
#include "../core/signal.h"
#include <SDL2/SDL_ttf.h>

#include "../core/fs.h"
#include "../core/pack.h"
#include <vector>

class PackManager;

namespace Ui {

class Window;

class LoadPackWidget : public SimpleContainer {
public:
    using FONT = FontStore::FONT;
    LoadPackWidget(int x, int y, int w, int h, FontStore *fontStore, Window *window=nullptr);

    void update();
    void updateInstalled();
    void reloadRemoteLists();
    void setPackManager(PackManager* packManager) { _packManager = packManager; }
    void focusFilter();
    void releaseFilterFocus();

    Signal<const fs::path&, const std::string&> onPackSelected;
    /// uid, version, download url, sha256
    Signal<const std::string&, const std::string&, const std::string&, const std::string&> onRemoteVersionSelected;

    Signal<> onManageListsRequested;

    virtual void setSize(Size size) override; // TODO: have more intelligent hbox instead

protected:
    enum class Mode { Local, Remote };

    FontStore *_fontStore;
    FONT _font;
    FONT _smallFont;

    Window *_window;
    PackManager *_packManager = nullptr;
    VBox *_root = nullptr;
    HBox *_filterBar = nullptr;
    TextField *_filter = nullptr;
    Button *_btnClearFilter = nullptr;
    Button *_btnSwitchList = nullptr;
    Button *_btnLists = nullptr;
    int _btnBarHeight = 0;
    ImageButton *_btnOpenFolder = nullptr;

    ScrollVBox *_packs;
    VBox *_variants; // variants (local) or versions (remote)
    HBox *_main;
    Label *_curPackLabel = nullptr;
    Label *_curVariantLabel = nullptr;
    Label *_curPackHover = nullptr;
    bool _disableHoverSelect = false;
    std::vector<Pack::Info> _availablePacks;
    std::vector<Pack::Info> _remotePacks;

    Mode _mode = Mode::Local; // local list is the default
    bool _remoteLoaded = false;
    bool _remoteLoading = false;
    unsigned _remoteGen = 0; // invalidates stale async version requests
    unsigned _remoteLoadGen = 0; // invalidates stale async pack list requests

    void refreshPacks();
    void setMode(Mode mode);
    void loadRemotePacks();
    void showRemoteVersions(const Pack::Info& pack);

    static constexpr int BTN_SWITCH_WIDTH = 80;
    static constexpr int BTN_LISTS_WIDTH = 64;

    static constexpr Color PACK_BG_DEFAULT = {32, 32, 32};
    static constexpr Color PACK_BG_ACTIVE = {32, 128, 32};
    static constexpr Color PACK_BG_HOVER = {64, 64, 64};
    static constexpr Color PACK_BG_ACTIVE_HOVER = {64, 160, 64};
    static constexpr Color VARIANT_BG_DEFAULT = {64, 64, 64};
    static constexpr Color VARIANT_BG_HOVER = {96, 96, 96};
};

} // namespace Ui

#endif /* _UI_LOADPACKWIDGET_H */
