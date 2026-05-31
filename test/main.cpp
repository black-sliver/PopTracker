#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "../src/core/assets.h"
#include "../src/http/http.h"

#ifdef _WIN32 // define macro function to setup mocking of Windows APIs
#    include <gmock-win32.h>
#    define SETUP_MOCK() \
    const gmock_win32::init_scope gmockWin32{ }
#else // no special setup required on non-windows yet
#    define SETUP_MOCK() do {} while (0)
#endif

int main(int argc, char **argv) {
    printf("Running main() from %s\n", __FILE__);

    HTTP::certFile = asset("cacert.pem").u8string();

    SETUP_MOCK();

    testing::InitGoogleTest(&argc, argv);
    int res = RUN_ALL_TESTS();
    SDL_Quit();
    IMG_Quit();
    return res;
}
