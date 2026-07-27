#include <memory>
#include <gtest/gtest.h>

#include "../../lib/luaglue/lua_include.h"
#include "../../src/ui/item.h"
#include "../../src/ui/trackerview.h"
#include "../uilib/font_helper.h"

using nlohmann::json;

constexpr char PACK_PATH[] = "examples/rules_test";
constexpr int RENDER_WIDTH = 32;
constexpr int RENDER_HEIGHT = 32;

class TestTrackerView : public Ui::TrackerView {
public:
    explicit TestTrackerView(Tracker* tracker)
        : TrackerView(0, 0, 0, 0, tracker, "default", &fontStore)
    {
    }

    std::unique_ptr<Ui::Item> itemFromJson(json&& j)
    {
        const auto item = JsonItem::FromJSON(j);
        return std::unique_ptr<Ui::Item>(makeItem(0, 0, 0, 0, item, -1, -1));
    }
};

static std::string getPixelData(const SDL_Surface* surface)
{
    return std::string(static_cast<const char*>(surface->pixels), surface->h * surface->pitch);
}

TEST(TrackerViewMakeItem, EmptyFallBackToImg)
{
    SDL_Surface* surface = SDL_CreateRGBSurface(0, RENDER_WIDTH, RENDER_HEIGHT, 32, 0, 0, 0, 0);
    if (!surface)
        throw std::runtime_error("failed to create surface");
    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    if (!renderer) {
        SDL_FreeSurface(surface);
        throw std::runtime_error("failed to create renderer");
    }
    lua_State* L = luaL_newstate();

    {
        Pack pack(PACK_PATH);
        Tracker tracker(&pack, L);
        TestTrackerView v(&tracker);
        const auto item1 = v.itemFromJson({
            {"type", "toggle"},
            {"img", "images/items/a.png"},
            {"disabled_img", "images/items/b.png"},
            {"disabled_img_mods", ""},
        });
        const auto item2 = v.itemFromJson({
            {"type", "toggle"},
            {"img", "images/items/a.png"},
            {"disabled_img", ""},
            {"disabled_img_mods", ""},
        });

        SDL_RenderClear(renderer);
        item1->render(renderer, 0, 0);
        const std::string data1Off = getPixelData(surface);

        SDL_RenderClear(renderer);
        item1->setStage(1, 0);
        item1->render(renderer, 0, 0);
        const std::string data1On = getPixelData(surface);

        SDL_RenderClear(renderer);
        item2->render(renderer, 0, 0);
        const std::string data2Off = getPixelData(surface);

        SDL_RenderClear(renderer);
        item2->setStage(1, 0);
        item2->render(renderer, 0, 0);
        const std::string data2On = getPixelData(surface);

        EXPECT_EQ(data1On, data2On) << "expected render of same img to be identical";
        EXPECT_NE(data1On, data1Off) << "expected render of different img to be different";
        EXPECT_EQ(data2On, data2Off) << "expected missing disabled_img to fall back to img";
    }

    lua_close(L);
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
}
