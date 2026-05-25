#include <chrono>
#include <thread>
#include <gtest/gtest.h>
#include "../../src/uilib/assetimagefuture.hpp"

constexpr char MAP_NAME[] = "icon512.png";
constexpr int ICON_WIDTH = 512;
constexpr int ICON_HEIGHT = 512;

TEST(AssetImageFuture, GetSurface) {
    Ui::AssetImageFuture future{MAP_NAME};
    SDL_Surface* surf = future.getSurface();
    ASSERT_TRUE(surf);
    EXPECT_EQ(surf->w, ICON_WIDTH);
    EXPECT_EQ(surf->h, ICON_HEIGHT);
    EXPECT_TRUE(surf->pixels);
    EXPECT_GE(surf->refcount, 0);
    SDL_FreeSurface(surf);
}

TEST(AssetImageFuture, IsSurfaceDone) {
    Ui::AssetImageFuture future{MAP_NAME};
    for (int i = 0; i < 1000; i++) {
        if (future.isSurfaceDone())
            return; // destructor should Get and Free it
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    FAIL() << "Did not finish loading in 1s";
}

TEST(AssetImageFuture, GetSize) {
    Ui::AssetImageFuture future{MAP_NAME};
    const Ui::Size size = future.getSize();
    EXPECT_EQ(size.width, ICON_WIDTH);
    EXPECT_EQ(size.height, ICON_HEIGHT);
}

TEST(AssetImageFuture, IsSizeDone) {
    Ui::AssetImageFuture future{MAP_NAME};
    for (int i = 0; i < 1000; i++) {
        if (future.isSizeDone())
            return; // destructor should Get and Free it
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    FAIL() << "Did not finish loading in 1s";
}

TEST(AssetImageFuture, CancelAlreadyDone) {
    Ui::AssetImageFuture future{MAP_NAME};
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

TEST(AssetImageFuture, CancelQueued) {
    Ui::AssetImageFuture future1{MAP_NAME};
    Ui::AssetImageFuture future2{MAP_NAME};
    future2.cancel(); // we hopefully get here before future1 is done
    SDL_Surface* surf1 = future1.getSurface();
    SDL_Surface* surf2 = future2.getSurface();
    EXPECT_TRUE(surf1);
    EXPECT_FALSE(surf2);
    SDL_FreeSurface(surf1);
    SDL_FreeSurface(surf2);
}

TEST(AssetImageFuture, Prioritize) {
    Ui::AssetImageFuture future1{MAP_NAME};
    Ui::AssetImageFuture future2{MAP_NAME};
    Ui::AssetImageFuture future3{MAP_NAME};
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

TEST(AssetImageFuture, NotAnImage) {
    Ui::AssetImageFuture future{"cacert.pem"};
    const Ui::Size size = future.getSize();
    EXPECT_EQ(size.width, 0);
    EXPECT_EQ(size.height, 0);
    SDL_Surface* surf = future.getSurface();
    EXPECT_FALSE(surf);
    SDL_FreeSurface(surf);
}

TEST(AssetImageFuture, Missing) {
    Ui::AssetImageFuture future{"missing"};
    const Ui::Size size = future.getSize();
    EXPECT_EQ(size.width, 0);
    EXPECT_EQ(size.height, 0);
    SDL_Surface* surf = future.getSurface();
    EXPECT_FALSE(surf);
    SDL_FreeSurface(surf);
}
