#include "loadpackwidget.h"
#include "../packmanager/packmanager.h"
#include "../core/pack.h"
#include "../core/assets.h"
#include "../core/util.h"
#include "../uilib/hbox.h"
#include "../uilib/vbox.h"
#include "../uilib/scrollvbox.h"
#include "../uilib/label.h"
#include "../uilib/imagebutton.h"
#include "defaults.h" // DEFAULT_FONT_*
#include <SDL2/SDL.h>
#include <fmt/format.h>
#include <cctype>
#include <algorithm>

namespace Ui {

static std::string urlEncode(const std::string& in)
{
    std::string out;
    out.reserve(in.size());
    for (unsigned char c : in) {
        if (isalnum(c) || c=='-' || c=='_' || c=='.' || c=='~' || c=='/')
            out += (char)c;
        else
            out += fmt::format("%{:02X}", c);
    }
    return out;
}
static void openPackDirectory()
{
    for (const auto& searchPath : Pack::getSearchPaths()) {
        fs::error_code ec;
        if (!fs::is_directory(searchPath, ec))
            continue;
        fs::path dir = fs::absolute(searchPath, ec);
        if (ec)
            continue;
        dir.make_preferred();
        std::string url = "file://" + urlEncode(dir.u8string());
        if (SDL_OpenURL(url.c_str()) == 0)
            return;
        fprintf(stderr, "LoadPackWidget: could not open pack directory '%s'\n", sanitize_print(searchPath).c_str());
    }
}

LoadPackWidget::LoadPackWidget(int x, int y, int w, int h, FontStore *fontStore, Window *window)
        : SimpleContainer(x,y,w,h), _fontStore(fontStore), _window(window)
{
    _font = _fontStore->getFont(DEFAULT_FONT_NAME, DEFAULT_FONT_SIZE);
    _smallFont = _fontStore->getFont(DEFAULT_FONT_NAME, DEFAULT_FONT_SIZE - 2);
    if (_font && !_smallFont) _smallFont = _font;

    onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_RIGHT) {
            setVisible(false); // TODO: fire onAbort ?
            if (_filter) _filter->releaseFocus();
        }
    }};

    auto packs = new ScrollVBox(0,0,0,0);
    packs->setGrow(1,1);
    packs->setPadding(0);
    packs->setSpacing(1);
    _packs = packs;

    auto variants = new VBox(0,0,0,0);
    variants->setGrow(1,1);
    variants->setPadding(0);
    variants->setSpacing(1);
    _variants = variants;

    auto hbox = new HBox(0,0,0,0);
    hbox->setGrow(1,1);
    hbox->addChild(packs);
    hbox->addChild(variants);
    hbox->setSpacing(1);
    hbox->setPadding(2);
    _main = hbox;

    auto filter = new TextField(0,0,0,0, _font, window);
    filter->setGrow(1,0);
    filter->setPlaceholder("Filter packs...");
    filter->setBackground({16,16,16});
    _filter = filter;

    auto btnClear = new Button(0,0,0,0, _font, "Clear");
    btnClear->setGrow(0,0);
    btnClear->setMinSize({64, btnClear->getAutoHeight()+4});
    _btnClearFilter = btnClear;

    btnClear->onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_LEFT) {
            _filter->clear();
        }
    }};

    // switches between local and remote list
    auto btnSwitch = new Button(0,0,0,0, _font, "Remote");
    btnSwitch->setGrow(0,0);
    _btnBarHeight = btnSwitch->getAutoHeight()+4;
    btnSwitch->setMinSize({BTN_SWITCH_WIDTH, _btnBarHeight});
    btnSwitch->setSize({BTN_SWITCH_WIDTH, _btnBarHeight});
    _btnSwitchList = btnSwitch;

    btnSwitch->onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_LEFT) {
            setMode(_mode == Mode::Local ? Mode::Remote : Mode::Local);
        }
    }};

    // opens the remote pack list manager
    auto btnLists = new Button(0,0,0,0, _font, "Lists");
    btnLists->setGrow(0,0);
    btnLists->setMinSize({BTN_LISTS_WIDTH, _btnBarHeight});
    btnLists->setSize({BTN_LISTS_WIDTH, _btnBarHeight});
    _btnLists = btnLists;

    btnLists->onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_LEFT) {
            onManageListsRequested.emit(this);
        }
    }};

    auto btnOpenFolder = new ImageButton(0,0,24,0, asset("closed.png"));
    btnOpenFolder->setGrow(0,0);
    btnOpenFolder->setMinSize({24, 0});
    _btnOpenFolder = btnOpenFolder;

    btnOpenFolder->onClick += {this, [](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_LEFT) {
            openPackDirectory();
        }
    }};

    filter->onTextChanged += {this, [this](void*, const std::string&) {
        refreshPacks();
    }};

    auto filterBar = new HBox(0,0,0,0);
    filterBar->setGrow(1,0);
    filterBar->setPadding(2);
    filterBar->setSpacing(4);
    filterBar->addChild(filter);
    filterBar->addChild(btnClear);
    filterBar->addChild(btnLists);
    filterBar->addChild(btnSwitch);
    filterBar->addChild(btnOpenFolder);
    filterBar->setHeight(filter->getHeight());
    _filterBar = filterBar;

    auto root = new VBox(0,0,0,0);
    root->setGrow(1,1);
    root->setPadding(0);
    root->setSpacing(1);
    root->addChild(filterBar);
    root->addChild(hbox);
    _root = root;
    addChild(root);

    _packs->onMouseLeave += {this, [this](void*) {
        if (_disableHoverSelect && _curPackHover) {
            if (_curPackHover == _curPackLabel) {
                _curPackHover->setBackground(PACK_BG_ACTIVE);
            } else {
                _curPackHover->setBackground(PACK_BG_DEFAULT);
            }
        }
    }};
}

static std::string toLower(std::string s) {
    for (auto& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

void LoadPackWidget::focusFilter()
{
    if (_filter) _filter->grabFocus();
}

void LoadPackWidget::releaseFilterFocus()
{
    if (_filter) _filter->releaseFocus();
}

void LoadPackWidget::update()
{
    _availablePacks = Pack::ListAvailable();
    _remotePacks.clear();
    _remoteLoaded = false;
    _remoteLoading = false;
    if (_filter) _filter->clear();
    if (_mode == Mode::Remote)
        loadRemotePacks();
    else
        refreshPacks();
}

void LoadPackWidget::updateInstalled()
{
    // rescan installed packs; installed packs vanish from the remote list by uid in refreshPacks()
    _availablePacks = Pack::ListAvailable();
    refreshPacks();
}

void LoadPackWidget::setMode(Mode mode)
{
    if (mode == _mode) return;
    _mode = mode;
    _btnSwitchList->setText(_mode == Mode::Local ? "Remote" : "Local");
    _btnSwitchList->setMinSize({BTN_SWITCH_WIDTH, _btnBarHeight});
    _btnSwitchList->setSize({BTN_SWITCH_WIDTH, _btnBarHeight});
    _filterBar->relayout();
    if (_filter) _filter->clear();
    if (_mode == Mode::Remote)
        loadRemotePacks();
    else
        refreshPacks();
}

void LoadPackWidget::reloadRemoteLists()
{
    ++_remoteLoadGen;
    _remotePacks.clear();
    _remoteLoaded = false;
    _remoteLoading = false;
    if (_mode == Mode::Remote)
        loadRemotePacks();
}

void LoadPackWidget::loadRemotePacks()
{
    if (_remoteLoaded || _remoteLoading || !_packManager || !_packManager->hasRepositories()) {
        refreshPacks(); // nothing to fetch (or already fetching): just show current state
        return;
    }
    _remoteLoading = true;
    refreshPacks(); // shows "Loading ..."
    const unsigned gen = ++_remoteLoadGen;
    _packManager->refreshAvailablePacks([this, gen](const nlohmann::json& packs) {
        if (gen != _remoteLoadGen) return;
        _remoteLoading = false;
        _remoteLoaded = true;
        _remotePacks.clear();
        for (const auto& item : packs.items()) {
            const auto& data = item.value();
            Pack::Info info;
            info.uid = item.key();
            info.packName = data.value("name", info.uid);
            info.gameName = info.packName;
            info.platform = data.value("platform", "");
            info.author = data.value("author", "");
            _remotePacks.push_back(std::move(info));
        }
        std::sort(_remotePacks.begin(), _remotePacks.end(), [](const Pack::Info& a, const Pack::Info& b) {
            const auto la = toLower(a.packName);
            const auto lb = toLower(b.packName);
            return la != lb ? la < lb : a.uid < b.uid;
        });
        refreshPacks();
    });
}

void LoadPackWidget::showRemoteVersions(const Pack::Info& pack)
{
    const unsigned gen = ++_remoteGen;

    auto addMessage = [this](const char* text) {
        auto lbl = new Label(0,0,0,0, _font, text);
        lbl->setGrow(1,0);
        lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
        lbl->setMinSize({64,lbl->getAutoHeight()});
        lbl->setSize({_variants->getWidth(),32});
        lbl->setBackground(VARIANT_BG_DEFAULT);
        _variants->addChild(lbl);
    };
    auto finishLayout = [this]() {
        auto spacer = new Label(0, 0, 0, 0, nullptr, "");
        spacer->setGrow(1,1);
        _variants->addChild(spacer);
        _main->relayout();
        _variants->relayout();
        _main->relayout();
    };

    _curVariantLabel = nullptr;
    _variants->clearChildren();
    addMessage(" Loading versions...");
    finishLayout();

    if (!_packManager) return;
    const std::string uid = pack.uid;
    const std::string listName = _packManager->getAvailablePackListName(uid);
    _packManager->getPackVersions(uid, [this, gen, uid, listName, addMessage, finishLayout]
            (bool ok, const std::vector<PackManager::RemoteVersion>& versions) {
        if (gen != _remoteGen) return; // user selected something else / list was rebuilt
        _curVariantLabel = nullptr;
        _variants->clearChildren();
        if (!ok || versions.empty()) {
            addMessage(" No downloadable versions found.");
        } else {
            for (const auto& v : versions) {
                std::string versionText = " " + v.version;
                if (!listName.empty())
                    versionText += "  (" + listName + ")";
                auto lbl = new Label(0,0,0,0, _font, versionText); // TODO: button instead of label
                lbl->setGrow(1,0);
                lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
                lbl->setMinSize({64,lbl->getAutoHeight()});
                lbl->setSize({_variants->getWidth(),32});
                lbl->setBackground(VARIANT_BG_DEFAULT);
                _variants->addChild(lbl);
                lbl->onMouseLeave += {this,[this](void *s) {
                    if (_curVariantLabel != s) return;
                    _curVariantLabel->setBackground(VARIANT_BG_DEFAULT);
                    _curVariantLabel = nullptr;
                }};
                lbl->onMouseEnter += {this,[this](void *s, int, int, unsigned) {
                    if (!s || s==_curVariantLabel) return;
                    if (_curVariantLabel) _curVariantLabel->onMouseLeave.emit(_curVariantLabel);
                    _curVariantLabel = (Label*)s;
                    _curVariantLabel->setBackground(VARIANT_BG_HOVER);
                }};
                lbl->onClick += {this,[this,uid,v](void*, int, int, int button) {
                    if (button == MouseButton::BUTTON_LEFT)
                        onRemoteVersionSelected.emit(this, uid, v.version, v.url, v.sha256);
                }};
            }
        }
        finishLayout();
    });
}

void LoadPackWidget::refreshPacks()
{
    ++_remoteGen; // drop pending version requests
    _packs->clearChildren();
    _variants->clearChildren();
    _curPackHover = nullptr;
    _curPackLabel = nullptr;
    _curVariantLabel = nullptr;
    _disableHoverSelect = false;

    const bool remote = (_mode == Mode::Remote);

    std::string filter;
    if (_filter) filter = toLower(_filter->getText());

    std::vector<Pack::Info> packs;
    if (remote) {
        // packs disappear from the remote list once installed (matched by UID)
        for (const auto& r : _remotePacks) {
            const bool installed = std::any_of(_availablePacks.begin(), _availablePacks.end(),
                    [&r](const Pack::Info& p) { return p.uid == r.uid; });
            if (!installed) packs.push_back(r);
        }
    } else {
        packs = _availablePacks;
    }

    int shown = 0;
    for (auto& pack : packs) {
        // "<name> <version> by <author>"; remote packs have no version, local packs may have no author
        std::string packLabel = " " + pack.packName;
        if (!pack.version.empty())
            packLabel += " " + pack.version;

        const std::string authorLabel =
            pack.author.empty() ? "" : "Made by " + pack.author;

        if (!filter.empty()) {
            const bool match = toLower(packLabel + " " + authorLabel).find(filter)
                            != std::string::npos
                            || toLower(pack.gameName).find(filter) != std::string::npos;
            if (!match)
                continue;
        }

        shown++;

        auto row = new HBox(0, 0, 0, 0);
        row->setGrow(1, 0);
        row->setPadding(0);
        row->setSpacing(0);
        row->setHeight(32);

        auto lbl = new Label(0, 0, 0, 0, _font, packLabel);
        lbl->setGrow(1, 0);
        lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
        lbl->setMinSize({64, lbl->getAutoHeight()});
        lbl->setBackground(PACK_BG_DEFAULT);

        auto author = new Label(0, 0, 0, 0, _smallFont, authorLabel);
        author->setGrow(0, 0);
        author->setTextAlignment(Label::HAlign::RIGHT, Label::VAlign::MIDDLE);
        author->setMinSize({0, author->getAutoHeight()});
        author->setBackground(PACK_BG_DEFAULT);

        lbl->setSize({0, 32});
        author->setSize({0, 32});

        row->addChild(lbl);
        row->addChild(author);
        _packs->addChild(row);

        auto setRowBackground = [lbl, author](Color color) {
            lbl->setBackground(color);
            author->setBackground(color);
        };

        if (remote) {
            row->onMouseEnter += {this, [this, lbl, setRowBackground](void*, int, int, unsigned) {
                if (lbl != _curPackLabel)
                    setRowBackground(PACK_BG_HOVER);
            }};

            row->onMouseLeave += {this, [this, lbl, setRowBackground](void*) {
                if (lbl != _curPackLabel)
                    setRowBackground(PACK_BG_DEFAULT);
            }};

            row->onClick += {this, [this, pack, lbl, setRowBackground](void*, int, int, int button) {
                if (button != MouseButton::BUTTON_LEFT)
                    return;

                if (_curPackLabel && _curPackLabel != lbl)
                    _curPackLabel->setBackground(PACK_BG_DEFAULT);

                _curPackLabel = lbl;
                setRowBackground(PACK_BG_ACTIVE_HOVER);
                showRemoteVersions(pack);
            }};

            continue;
        }

        row->onMouseEnter += {this, [this, pack, lbl, setRowBackground](void*, int, int, unsigned) {
            if (_curPackHover != lbl) {
                if (_curPackHover == _curPackLabel && _disableHoverSelect)
                    _curPackHover->setBackground(PACK_BG_ACTIVE);
                else if (_curPackHover)
                    _curPackHover->setBackground(PACK_BG_DEFAULT);

                _curPackHover = lbl;

                if (_curPackHover == _curPackLabel && _disableHoverSelect)
                    setRowBackground(PACK_BG_ACTIVE_HOVER);
                else
                    setRowBackground(PACK_BG_HOVER);
            }

            if (_disableHoverSelect || _curPackLabel == lbl)
                return;

            if (_curPackLabel)
                _curPackLabel->setBackground(PACK_BG_DEFAULT);

            _curPackLabel = lbl;
            _variants->clearChildren();

            for (auto& variant : pack.variants) {
                auto variantLabel = new Label(0, 0, 0, 0, _font, " " + variant.name);
                variantLabel->setGrow(1, 0);
                variantLabel->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
                variantLabel->setMinSize({64, variantLabel->getAutoHeight()});
                variantLabel->setSize({_variants->getWidth(), 32});
                variantLabel->setBackground(VARIANT_BG_DEFAULT);
                _variants->addChild(variantLabel);

                variantLabel->onMouseLeave += {this, [this](void* s) {
                    if (_curVariantLabel != s)
                        return;
                    _curVariantLabel->setBackground(VARIANT_BG_DEFAULT);
                    _curVariantLabel = nullptr;
                }};

                auto path = pack.path;

                variantLabel->onMouseEnter += {this, [this](void* s, int, int, unsigned) {
                    if (!s || s == _curVariantLabel)
                        return;
                    if (_curVariantLabel)
                        _curVariantLabel->onMouseLeave.emit(_curVariantLabel);
                    _curVariantLabel = (Label*)s;
                    _curVariantLabel->setBackground(VARIANT_BG_HOVER);
                }};

                variantLabel->onClick += {this, [this, path, variant](void*, int, int, int button) {
                    if (button == MouseButton::BUTTON_LEFT)
                        onPackSelected.emit(this, path, variant.variant);
                }};
            }

            auto spacer = new Label(0, 0, 0, 0, nullptr, "");
            spacer->setGrow(1, 1);
            _variants->addChild(spacer);

            _main->relayout();
            _variants->relayout();
            _main->relayout();
        }};

        row->onClick += {this, [this, row, setRowBackground](void*, int, int, int button) {
        if (button != MouseButton::BUTTON_LEFT)
                return;

            _disableHoverSelect = false;
            row->onMouseEnter.emit(row, 0, 0, (unsigned)button);
            _disableHoverSelect = true;

            if (_curPackLabel)
                setRowBackground(PACK_BG_ACTIVE_HOVER);
        }};
    }

    if (shown == 0) {
        std::string msg;
        if (remote) {
            if (_remoteLoading)
                msg = "Loading remote pack list...";
            else if (!_packManager || !_packManager->hasRepositories())
                msg = "No remote pack lists enabled.\n"
                      "Click \"Lists\" to select or add one.";
            else if (!_remotePacks.empty() || !filter.empty())
                msg = "No packs found!";
            else
                msg = "No remote packs available.";
        } else {
            msg = _availablePacks.empty() ?
                "No packs installed!\nDrag & drop packs into the window to install them." :
                "No packs found!";
        }
        auto* lbl = new Label(0, 0, 0, 0, _font, msg);
        lbl->setGrow(1,1);
        lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
        lbl->setMinSize(lbl->getAutoSize());
        lbl->setSize({_size.width/2,32}); // TODO; hbox with even split instead
        _packs->addChild(lbl);
        auto* spacer = new Label(0, 0, 0, 0, nullptr, "");
        spacer->setGrow(1,1);
        _packs->addChild(spacer);
    }
    // Pin both columns to half width and do the same 3-step relayout the hover handler does,
    // otherwise the columns keep stale sizes until the first hover triggers it.
    // TODO: have this be done automatically
    _packs->setWidth(_size.width/2 - 1);
    _variants->setWidth(_size.width/2 - 1);
    _main->relayout();
    _variants->relayout();
    _main->relayout(); // FIXME: this should not be required
}

void LoadPackWidget::setSize(Size size)
{
    SimpleContainer::setSize(size);
    // TODO: have more intelligent hbox instead
    _packs->setWidth(size.width/2-1);
    _variants->setWidth(size.width/2-1);
    _main->relayout();
}


} // namespace
