// very basic tests for some coverage with ASAN and UBSAN

#include <chrono>
#include <memory>
#include <thread>
#include <utility>
#include <gtest/gtest.h>
#include <SDL2/SDL.h>
#include "../../src/core/pack.h"
#include "../../src/ui/item.h"
#include "../../src/ui/packimagefuture.hpp"
#include "../uilib/font_helper.h"

class ItemRenderTest : public testing::Test {
protected:
    static constexpr char PACK_PATH[] = "examples/rules_test";
    static constexpr char SMALL_IMAGE_NAME[] = "images/items/a.png";
    static constexpr char BIG_IMAGE_NAME[] = "images/maps/example.png";
    static constexpr int RENDER_WIDTH = 128;
    static constexpr int RENDER_HEIGHT = 128;

    SDL_Renderer* _renderer = nullptr;
    SDL_Surface* _surface = nullptr;

    ItemRenderTest()
    {
        _surface = SDL_CreateRGBSurface(0, RENDER_WIDTH, RENDER_HEIGHT, 32, 0, 0, 0, 0);
        if (!_surface)
            throw std::runtime_error("failed to create surface");
        if (!surfaceEmpty()) {
            SDL_FreeSurface(_surface);
            _surface = nullptr;
            throw std::runtime_error("new surface not empty");
        }
        _renderer = SDL_CreateSoftwareRenderer(_surface);
        if (!_renderer) {
            SDL_FreeSurface(_surface);
            _surface = nullptr;
            throw std::runtime_error("failed to create renderer");
        }
    }

    bool surfaceEmpty() const noexcept
    {
        auto* pixels = static_cast<const uint32_t*>(_surface->pixels);
        const size_t size = static_cast<size_t>(_surface->pitch) * static_cast<size_t>(_surface->h) / 4;
        SDL_LockSurface(_surface);
        for (size_t i = 0; i < size; i++) {
            if (pixels[i]) {
                SDL_UnlockSurface(_surface);
                return false;
            }
        }
        SDL_UnlockSurface(_surface);
        return true;
    }

    void clear() const noexcept
    {
        SDL_RenderClear(_renderer);
    }

public:
    ~ItemRenderTest() override
    {
        SDL_DestroyRenderer(_renderer);
        SDL_FreeSurface(_surface);
    }
};

TEST_F(ItemRenderTest, SmallStageFuture) {
    Pack pack(PACK_PATH);
    ASSERT_TRUE(pack.hasFile(SMALL_IMAGE_NAME));
    Ui::Item item(0, 0, 0, 0, getDefaultFont());
    item.addStage(
        0,
        0,
        std::make_unique<Ui::PackImageFuture>(&pack, SMALL_IMAGE_NAME),
        SMALL_IMAGE_NAME
    );
    // first render should wait for small image and render it immediately
    item.render(_renderer, 0, 0);
    EXPECT_FALSE(surfaceEmpty());
    // add the same image again (should use cache)
    item.addStage(
        1,
        0,
        std::make_unique<Ui::PackImageFuture>(&pack, SMALL_IMAGE_NAME),
        SMALL_IMAGE_NAME
    );
    item.setStage(1, 0);
    // should render immediately as well
    clear();
    EXPECT_TRUE(surfaceEmpty());
    item.render(_renderer, 0, 0);
    EXPECT_FALSE(surfaceEmpty());
}

TEST_F(ItemRenderTest, BigStageFuture) {
    Pack pack(PACK_PATH);
    ASSERT_TRUE(pack.hasFile(BIG_IMAGE_NAME));
    Ui::Item item(0, 0, 0, 0, getDefaultFont());
    item.addStage(
        0,
        0,
        std::make_unique<Ui::PackImageFuture>(&pack, BIG_IMAGE_NAME),
        BIG_IMAGE_NAME
    );
    // render should eventually render something
    for (int i = 0; i < 100; i++) {
        item.render(_renderer, 0, 0);
        if (!surfaceEmpty())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    // rendering again should render something
    clear();
    item.render(_renderer, 0, 0);
    EXPECT_FALSE(surfaceEmpty());
}

TEST_F(ItemRenderTest, MissingStageFuture) {
    Pack pack(PACK_PATH);
    ASSERT_FALSE(pack.hasFile("missing"));
    // item with fixed size
    Ui::Item item(0, 0, 64, 64, getDefaultFont());
    // add stage
    item.addStage(
        0,
        0,
        std::make_unique<Ui::PackImageFuture>(&pack, "missing"),
        "missing"
    );
    // render will render nothing and print a warning/error message
    item.render(_renderer, 0, 0);
    EXPECT_TRUE(surfaceEmpty());
}

TEST_F(ItemRenderTest, SmallOverrideFuture) {
    Pack pack(PACK_PATH);
    ASSERT_TRUE(pack.hasFile(SMALL_IMAGE_NAME));
    Ui::Item item(0, 0, 0, 0, getDefaultFont());
    // we create a future to get the correct size, but then we cancel it so the stage data is empty
    auto future = std::make_unique<Ui::PackImageFuture>(&pack, SMALL_IMAGE_NAME);
    future->cancel();
    item.addStage(
        0,
        0,
        std::move(future),
        SMALL_IMAGE_NAME
    );
    item.setImageOverride(
        [&pack]() {
            return std::make_unique<Ui::PackImageFuture>(&pack, SMALL_IMAGE_NAME);
        },
        SMALL_IMAGE_NAME,
        {}
    );
    // first render should wait for small image and render it immediately
    item.render(_renderer, 0, 0);
    EXPECT_FALSE(surfaceEmpty());
}

TEST_F(ItemRenderTest, BigOverrideFuture) {
    Pack pack(PACK_PATH);
    ASSERT_TRUE(pack.hasFile(BIG_IMAGE_NAME));
    Ui::Item item(0, 0, 0, 0, getDefaultFont());
    item.addStage(
        0,
        0,
        std::make_unique<Ui::PackImageFuture>(&pack, SMALL_IMAGE_NAME),
        SMALL_IMAGE_NAME
    );
    item.setImageOverride(
        [&pack]() {
            return std::make_unique<Ui::PackImageFuture>(&pack, BIG_IMAGE_NAME);
        },
        BIG_IMAGE_NAME,
        {}
    );
    // render should eventually render something
    for (int i = 0; i < 100; i++) {
        item.render(_renderer, 0, 0);
        if (!surfaceEmpty())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    // rendering again should render something
    clear();
    item.render(_renderer, 0, 0);
    EXPECT_FALSE(surfaceEmpty());
}

TEST_F(ItemRenderTest, MissingOverride) {
    Pack pack(PACK_PATH);
    Ui::Item item(0, 0, 0, 0, getDefaultFont());
    // add a stage
    item.addStage(
        0,
        0,
        std::make_unique<Ui::PackImageFuture>(&pack, SMALL_IMAGE_NAME),
        SMALL_IMAGE_NAME
    );
    // overwrite with missing, will print a warning/error message
    item.setImageOverride("", 0, "missing", {});
    // render will render blank
    item.render(_renderer, 0, 0);
    EXPECT_TRUE(surfaceEmpty());
}

TEST_F(ItemRenderTest, MissingOverrideFuture) {
    Pack pack(PACK_PATH);
    ASSERT_FALSE(pack.hasFile("missing"));
    Ui::Item item(0, 0, 0, 0, getDefaultFont());
    // add a stage
    item.addStage(
        0,
        0,
        std::make_unique<Ui::PackImageFuture>(&pack, SMALL_IMAGE_NAME),
        SMALL_IMAGE_NAME
    );
    // overwrite with missing future
    item.setImageOverride(
        [&pack]() {
            return std::make_unique<Ui::PackImageFuture>(&pack, "missing");
        },
        "missing",
        {}
    );
    // render will render blank and print a warning/error message
    item.render(_renderer, 0, 0);
    EXPECT_TRUE(surfaceEmpty());
}
