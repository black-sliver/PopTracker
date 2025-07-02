#include <gmock/gmock.h>
#include <gtest/gtest.h>
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

    HTTP::certfile = asset("cacert.pem").u8string();

    SETUP_MOCK();

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
