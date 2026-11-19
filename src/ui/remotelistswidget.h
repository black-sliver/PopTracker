#ifndef _UI_REMOTELISTSWIDGET_H
#define _UI_REMOTELISTSWIDGET_H

#include "../uilib/simplecontainer.h"
#include "../uilib/hbox.h"
#include "../uilib/vbox.h"
#include "../uilib/scrollvbox.h"
#include "../uilib/label.h"
#include "../uilib/button.h"
#include "../uilib/fontstore.h"
#include "../core/signal.h"
#include "../packmanager/packlist.h"
#include <vector>

class PackManager;

namespace Ui {

class RemoteListsWidget : public SimpleContainer {
public:
    using FONT = FontStore::FONT;
    RemoteListsWidget(int x, int y, int w, int h, FontStore *fontStore);

    void setPackManager(PackManager* packManager) { _packManager = packManager; }
    void setEntries(const std::vector<PackListEntry>& entries);
    const std::vector<PackListEntry>& getEntries() const { return _entries; }

    Signal<const std::vector<PackListEntry>&> onChanged;
    Signal<> onClosed;

    virtual void render(Renderer renderer, int offX, int offY) override;
    virtual void setSize(Size size) override;

protected:
    FontStore *_fontStore;
    FONT _font;
    FONT _smallFont;
    PackManager *_packManager = nullptr;

    VBox *_root = nullptr;
    ScrollVBox *_list = nullptr;
    Label *_lblTitle = nullptr;
    Label *_lblDescription = nullptr;
    Label *_lblNote = nullptr;

    std::vector<PackListEntry> _entries;
    bool _dirty = false;
    bool _validating = false;

    void rebuild();
    void addEntryRows(size_t index);
    void changed();
    void close();
    void addFromUrl();

    static constexpr Color ROW_BG = {32, 32, 32};
};

} // namespace Ui

#endif /* _UI_REMOTELISTSWIDGET_H */
