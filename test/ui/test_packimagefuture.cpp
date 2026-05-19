#include <chrono>
#include <thread>
#include <gtest/gtest.h>
#include "../../src/ui/packimagefuture.hpp"
#include "../../src/uilib/imghelper.h"

constexpr char PACK_PATH[] = "examples/rules_test";
constexpr char MAP_NAME[] = "images/maps/example.png";
constexpr int MAP_WIDTH = 600;
constexpr int MAP_HEIGHT = 600;
constexpr char SMALL_IMAGE_NAME[] = "images/items/a.png";

// TODO: test variant resolution for image loading
// TODO: also test image loading from zipped pack

TEST(PackImageFuture, GetSurface) {
    Pack pack(PACK_PATH);
    Ui::PackImageFuture future{&pack, MAP_NAME};
    SDL_Surface* surf = future.getSurface();
    ASSERT_TRUE(surf);
    EXPECT_EQ(surf->w, MAP_WIDTH);
    EXPECT_EQ(surf->h, MAP_HEIGHT);
    EXPECT_TRUE(surf->pixels);
    EXPECT_GE(surf->refcount, 0);
    SDL_FreeSurface(surf);
}

TEST(PackImageFuture, SmallImageCache) {
    Pack pack(PACK_PATH);
    Ui::PackImageFuture future1{&pack, SMALL_IMAGE_NAME};
    if (!Ui::isSmallImage(future1.getSize()))
        FAIL() << SMALL_IMAGE_NAME << " is not a small image";
    SDL_Surface* surf1 = future1.getSurface();
    ASSERT_TRUE(surf1);
    Ui::PackImageFuture future2{&pack, SMALL_IMAGE_NAME};
    SDL_Surface* surf2 = future2.getSurface();
    EXPECT_EQ(surf1, surf2);
    SDL_FreeSurface(surf1); // should be a no-op
    SDL_FreeSurface(surf2); // should be a no-op
}

TEST(PackImageFuture, IsSurfaceDone) {
    Pack pack(PACK_PATH);
    Ui::PackImageFuture future{&pack, MAP_NAME};
    for (int i = 0; i < 1000; i++) {
        if (future.isSurfaceDone())
            return; // destructor should Get and Free it
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    FAIL() << "Did not finish loading in 1s";
}

TEST(PackImageFuture, GetSize) {
    Pack pack(PACK_PATH);
    Ui::PackImageFuture future{&pack, MAP_NAME};
    const Ui::Size size = future.getSize();
    EXPECT_EQ(size.width, MAP_WIDTH);
    EXPECT_EQ(size.height, MAP_HEIGHT);
}

TEST(PackImageFuture, IsSizeDone) {
    Pack pack(PACK_PATH);
    Ui::PackImageFuture future{&pack, MAP_NAME};
    for (int i = 0; i < 1000; i++) {
        if (future.isSizeDone())
            return; // destructor should Get and Free it
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    FAIL() << "Did not finish loading in 1s";
}

TEST(PackImageFuture, CancelAlreadyDone) {
    Pack pack(PACK_PATH);
    Ui::PackImageFuture future{&pack, MAP_NAME};
    for (int i = 0; i < 1000; i++) {
        if (future.isSurfaceDone())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!future.isSurfaceDone())
        FAIL() << "Did not finish loading in 1s";
    future.cancel(); // this should free the surface
    SDL_Surface* surf = future.getSurface();
    EXPECT_FALSE(surf);
    SDL_FreeSurface(surf);
}

TEST(PackImageFuture, CancelQueued) {
    Pack pack(PACK_PATH);
    Ui::PackImageFuture future1{&pack, MAP_NAME};
    Ui::PackImageFuture future2{&pack, MAP_NAME};
    future2.cancel(); // we hopefully get here before future1 is done
    SDL_Surface* surf1 = future1.getSurface();
    SDL_Surface* surf2 = future2.getSurface();
    EXPECT_TRUE(surf1);
    EXPECT_FALSE(surf2);
    SDL_FreeSurface(surf1);
    SDL_FreeSurface(surf2);
}

TEST(PackImageFutureTest, Prioritize) {
    Pack pack(PACK_PATH);
    Ui::PackImageFuture future1{&pack, MAP_NAME};
    Ui::PackImageFuture future2{&pack, MAP_NAME};
    Ui::PackImageFuture future3{&pack, MAP_NAME};
    future3.prioritize(); // run future3 before future2
    future3.prioritize(); // this should do nothing
    future2.prioritize(); // this should do nothing
    for (int i = 0; i < 100000; i++) {
        if (future2.isSurfaceDone()) {
            EXPECT_TRUE(future3.isSurfaceDone());
            return;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    FAIL() << "Did not finish loading in 1s";
}

TEST(PackImageFuture, NotAnImage) {
    Pack pack(PACK_PATH);
    Ui::PackImageFuture future{&pack, "scripts/init.lua"};
    const Ui::Size size = future.getSize();
    EXPECT_EQ(size.width, 0);
    EXPECT_EQ(size.height, 0);
    SDL_Surface* surf = future.getSurface();
    EXPECT_FALSE(surf);
    SDL_FreeSurface(surf);
}

TEST(PackImageFuture, Missing) {
    Pack pack(PACK_PATH);
    Ui::PackImageFuture future{&pack, "missing"};
    const Ui::Size size = future.getSize();
    EXPECT_EQ(size.width, 0);
    EXPECT_EQ(size.height, 0);
    SDL_Surface* surf = future.getSurface();
    EXPECT_FALSE(surf);
    SDL_FreeSurface(surf);
}
