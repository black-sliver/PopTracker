#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>
#include <gtest/gtest.h>

#include "../../lib/luaglue/lua_include.h"
#include "../../src/ui/mapwidget.h"
#include "../../src/ui/trackerview.h"
#include "../uilib/font_helper.h"

namespace {

constexpr char IMAGE_PATH[] = "examples/zoom-pan-test/images/map.png";
constexpr char PACK_PATH[] = "examples/zoom-pan-test";
constexpr int SURFACE_WIDTH = 240;
constexpr int SURFACE_HEIGHT = 120;

class TestTrackerView : public Ui::TrackerView {
public:
    explicit TestTrackerView(Tracker* tracker)
        : TrackerView(0, 0, SURFACE_WIDTH, SURFACE_HEIGHT, tracker, "tracker_default", &fontStore)
    {
    }

    Ui::MapWidget* map(const std::string& name, const size_t index = 0)
    {
        auto it = _maps.find(name);
        if (it == _maps.end() || index >= it->second.size())
            return nullptr;
        auto widget = it->second.begin();
        std::advance(widget, static_cast<long>(index));
        return *widget;
    }

    bool addDuplicateMapNode()
    {
        return addLayoutNode(this, LayoutNode::FromJSON({
            {"type", "map"},
            {"maps", {"map", "map"}},
        }));
    }
};

class SoftwareRenderer {
public:
    SoftwareRenderer()
    {
        surface = SDL_CreateRGBSurfaceWithFormat(0, SURFACE_WIDTH, SURFACE_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
        if (!surface)
            throw std::runtime_error("failed to create surface");
        renderer = SDL_CreateSoftwareRenderer(surface);
        if (!renderer) {
            SDL_FreeSurface(surface);
            throw std::runtime_error("failed to create renderer");
        }
    }

    ~SoftwareRenderer()
    {
        SDL_DestroyRenderer(renderer);
        SDL_FreeSurface(surface);
    }

    std::string render(Ui::MapWidget& widget)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        widget.render(renderer, 0, 0);
        SDL_RenderPresent(renderer);
        return std::string(static_cast<const char*>(surface->pixels), surface->h * surface->pitch);
    }

    SDL_Surface* surface = nullptr;
    SDL_Renderer* renderer = nullptr;
};

struct Bounds {
    int left = SURFACE_WIDTH;
    int top = SURFACE_HEIGHT;
    int right = -1;
    int bottom = -1;

    bool empty() const { return right < left || bottom < top; }
    int width() const { return empty() ? 0 : right - left + 1; }
    int height() const { return empty() ? 0 : bottom - top + 1; }
};

Bounds differenceBounds(const std::string& before, const std::string& after, const int pitch)
{
    Bounds bounds;
    for (int y = 0; y < SURFACE_HEIGHT; ++y) {
        for (int x = 0; x < SURFACE_WIDTH; ++x) {
            const size_t offset = static_cast<size_t>(y * pitch + x * 4);
            if (before.compare(offset, 4, after, offset, 4) != 0) {
                bounds.left = std::min(bounds.left, x);
                bounds.top = std::min(bounds.top, y);
                bounds.right = std::max(bounds.right, x);
                bounds.bottom = std::max(bounds.bottom, y);
            }
        }
    }
    return bounds;
}

TEST(MapWidgetMarker, SetUpdateAndClearState)
{
    SoftwareRenderer output;
    Ui::MapWidget widget(0, 0, 207, 100, IMAGE_PATH);
    const std::string baseline = output.render(widget);

    widget.setMarker("player", 100.25f, 50.5f);
    EXPECT_FALSE(output.render(widget) == baseline);
    widget.setMarker("player", 200.5f, 50.75f);
    const std::string moved = output.render(widget);
    Ui::MapWidget onlyMovedMarker(0, 0, 207, 100, IMAGE_PATH);
    onlyMovedMarker.setMarker("player", 200.5f, 50.75f);
    EXPECT_TRUE(moved == output.render(onlyMovedMarker));

    widget.setMarker("spectator", 300.0f, 100.0f);
    widget.clearMarker("player");
    Ui::MapWidget onlySpectator(0, 0, 207, 100, IMAGE_PATH);
    onlySpectator.setMarker("spectator", 300.0f, 100.0f);
    EXPECT_TRUE(output.render(widget) == output.render(onlySpectator));

    widget.clearMarkers();
    EXPECT_TRUE(output.render(widget) == baseline);
}

TEST(MapWidgetMarker, RenderFollowsZoomAndPanWithFixedSize)
{
    SoftwareRenderer output;
    Ui::MapWidget widget(10, 10, 207, 100, IMAGE_PATH);
    const std::string baseline = output.render(widget);

    widget.setMarker("player", 414.0f, 200.0f);
    const Bounds normal = differenceBounds(baseline, output.render(widget), output.surface->pitch);
    ASSERT_FALSE(normal.empty());

    widget.setZoom(2.0f);
    const std::string zoomBaseline = [&] {
        widget.clearMarkers();
        const std::string pixels = output.render(widget);
        widget.setMarker("player", 414.0f, 200.0f);
        return pixels;
    }();
    const Bounds zoomed = differenceBounds(zoomBaseline, output.render(widget), output.surface->pitch);
    EXPECT_LE(std::abs(zoomed.width() - normal.width()), 1);
    EXPECT_LE(std::abs(zoomed.height() - normal.height()), 1);
    EXPECT_LE(std::abs((zoomed.left + zoomed.right) - (normal.left + normal.right)), 1);
    EXPECT_LE(std::abs((zoomed.top + zoomed.bottom) - (normal.top + normal.bottom)), 1);

    widget.setPanCenter(300.0f, 200.0f);
    const std::string panBaseline = [&] {
        widget.clearMarkers();
        const std::string pixels = output.render(widget);
        widget.setMarker("player", 414.0f, 200.0f);
        return pixels;
    }();
    const Bounds panned = differenceBounds(panBaseline, output.render(widget), output.surface->pitch);
    EXPECT_GT(panned.left, zoomed.left);
    EXPECT_LE(std::abs(panned.width() - normal.width()), 1);
    EXPECT_LE(std::abs(panned.height() - normal.height()), 1);
}

TEST(MapWidgetMarker, WidgetClipContainsOffViewportMarker)
{
    SoftwareRenderer output;
    Ui::MapWidget widget(20, 20, 100, 80, IMAGE_PATH);
    const std::string baseline = output.render(widget);

    widget.setMarker("player", -10000.0f, -10000.0f);
    EXPECT_TRUE(output.render(widget) == baseline);
}

TEST(TrackerViewMapMarkerHint, ParsesJsonRoutesRemovesAndResetsWithoutSaving)
{
    (void)getDefaultFont();
    lua_State* L = luaL_newstate();
    ASSERT_NE(L, nullptr);

    {
        Pack pack(PACK_PATH);
        pack.setVariant("standard");
        Tracker tracker(&pack, L);
        ASSERT_TRUE(tracker.AddMaps("maps/maps.json"));
        ASSERT_TRUE(tracker.AddLocations("locations/locations.json"));
        ASSERT_TRUE(tracker.AddItems("items/items.json"));
        ASSERT_TRUE(tracker.AddLayouts("layouts/standard.json"));

        TestTrackerView view(&tracker);
        view.relayout();
        Ui::MapWidget* map = view.map("map");
        ASSERT_NE(map, nullptr);
        map->setPosition({10, 10});
        map->setSize({207, 100});
        map->setImage(IMAGE_PATH);

        SoftwareRenderer output;
        const std::string baseline = output.render(*map);

        const std::string defaultMarker = R"({"id":"player","x":414.5,"y":200.25})";
        const std::string redMarker = R"({"id":"player","x":414.5,"y":200.25,"appearance":{"type":"diamond","color":"#ff0000"}})";
        const std::string alphaMarker = R"({"id":"player","x":414.5,"y":200.25,"appearance":{"type":"diamond","color":"#8000ff00"}})";

        tracker.UiHint("MapMarker map", defaultMarker);
        const std::string defaultMarked = output.render(*map);
        EXPECT_FALSE(defaultMarked == baseline);

        tracker.UiHint("MapMarker map", redMarker);
        const std::string marked = output.render(*map);
        EXPECT_FALSE(marked == defaultMarked);

        tracker.UiHint("MapMarker map", alphaMarker);
        const std::string alphaMarked = output.render(*map);
        EXPECT_FALSE(alphaMarked == marked);

        // Set operations completely replace the appearance rather than retaining it.
        tracker.UiHint("MapMarker map", defaultMarker);
        EXPECT_TRUE(output.render(*map) == defaultMarked);
        tracker.UiHint("MapMarker map", redMarker);
        EXPECT_TRUE(output.render(*map) == marked);

        const auto savedHints = view.getHints();
        EXPECT_TRUE(std::none_of(savedHints.begin(), savedHints.end(), [](const auto& hint) {
            return hint.first.rfind("MapMarker ", 0) == 0;
        }));

        const std::vector<std::string> invalidValues = {
            "", "not JSON", "[]", "null", "{}",
            R"({"id":"","x":1,"y":2})", R"({"id":1,"x":1,"y":2})",
            R"({"id":"player","x":1})", R"({"id":"player","y":2})",
            R"({"id":"player","x":"1","y":2})", R"({"id":"player","x":true,"y":2})",
            R"({"id":"player","x":null,"y":2})", R"({"id":"player","x":[],"y":2})",
            R"({"id":"player","x":1,"y":"2"})", R"({"id":"player","x":1,"y":false})",
            R"({"id":"player","x":NaN,"y":2})", R"({"id":"player","x":Infinity,"y":2})",
            R"({"id":"player","x":-Infinity,"y":2})", R"({"id":"player","x":3.5e38,"y":2})",
            R"({"id":"player","x":1,"y":3.5e38})", R"({"id":"player","x":1e400,"y":2})",
            R"({"id":"player","remove":false})", R"({"id":"player","remove":"true"})",
            R"({"id":"player","remove":true,"x":1,"y":2})", R"({"id":"player","extra":true,"x":1,"y":2})",
            R"({"id":"player","x":1,"y":2,"label":"not yet supported"})",
            R"({"id":"player","id":"other","x":1,"y":2})",
            R"({"id":"player","x":1,"y":2,"appearance":null})",
            R"({"id":"player","x":1,"y":2,"appearance":{}})",
            R"({"id":"player","x":1,"y":2,"appearance":{"type":1}})",
            R"({"id":"player","x":1,"y":2,"appearance":{"type":"circle"}})",
            R"({"id":"player","x":1,"y":2,"appearance":{"type":"diamond","color":1}})",
            R"({"id":"player","x":1,"y":2,"appearance":{"type":"diamond","color":"ff0000"}})",
            R"({"id":"player","x":1,"y":2,"appearance":{"type":"diamond","color":"#f00"}})",
            R"({"id":"player","x":1,"y":2,"appearance":{"type":"diamond","color":"#ff000"}})",
            R"({"id":"player","x":1,"y":2,"appearance":{"type":"diamond","color":"#ff000g"}})",
            R"({"id":"player","x":1,"y":2,"appearance":{"type":"diamond","type":"diamond"}})",
            R"({"id":"player","x":1,"y":2,"appearance":{"type":"diamond","color":"#12ff0000","path":"images/player.png"}})",
        };
        for (const auto& value : invalidValues) {
            EXPECT_NO_THROW(tracker.UiHint("MapMarker map", value));
            EXPECT_TRUE(output.render(*map) == marked) << value;
        }

        tracker.UiHint("MapMarker other", R"({"id":"player","remove":true})");
        EXPECT_TRUE(output.render(*map) == marked);
        tracker.UiHint("MapMarker map[0]", R"({"id":"player","remove":true})");
        EXPECT_TRUE(output.render(*map) == baseline);

        // Coordinates remain valid when fractional or outside the map image.
        tracker.UiHint("MapMarker map[0]", R"({"id":"player","x":-4.5,"y":3.25})");
        EXPECT_FALSE(output.render(*map) == baseline);
        tracker.UiHint("reset", "reset");
        EXPECT_TRUE(output.render(*map) == baseline);
    }

    lua_close(L);
}

TEST(TrackerViewMapMarkerHint, UnnumberedTargetsAllAndNumberedTargetsOneInstance)
{
    (void)getDefaultFont();
    lua_State* L = luaL_newstate();
    ASSERT_NE(L, nullptr);

    {
        Pack pack(PACK_PATH);
        pack.setVariant("standard");
        Tracker tracker(&pack, L);
        ASSERT_TRUE(tracker.AddMaps("maps/maps.json"));

        TestTrackerView view(&tracker);
        ASSERT_TRUE(view.addDuplicateMapNode());
        Ui::MapWidget* first = view.map("map", 0);
        Ui::MapWidget* second = view.map("map", 1);
        ASSERT_NE(first, nullptr);
        ASSERT_NE(second, nullptr);
        for (Ui::MapWidget* map : {first, second}) {
            map->setPosition({0, 0});
            map->setSize({207, 100});
            map->setImage(IMAGE_PATH);
        }

        SoftwareRenderer output;
        const std::string firstBaseline = output.render(*first);
        const std::string secondBaseline = output.render(*second);

        tracker.UiHint("MapMarker map", R"({"id":"player","x":414,"y":200})");
        EXPECT_FALSE(output.render(*first) == firstBaseline);
        EXPECT_FALSE(output.render(*second) == secondBaseline);

        tracker.UiHint("MapMarker map", R"({"id":"player","remove":true})");
        tracker.UiHint("MapMarker map[0]", R"({"id":"player","x":414,"y":200})");
        EXPECT_FALSE(output.render(*first) == firstBaseline);
        EXPECT_TRUE(output.render(*second) == secondBaseline);
    }

    lua_close(L);
}

} // namespace
