# Automated PopTracker Tests

PRs to add tests are welcome.

We have 3 types of tests in mind:
- **Unit tests:** test individual code fragments using googletest, see [How to add Unit Tests](#how-to-add-unit-tests).
- **Render tests:** load packs and verify render output using `SDL_VIDEODRIVER=dummy` and a screenshot
- **High level tests:** stuff that should be tested end-to-end may be hard to do with googletest because we'd need to
  mock all of PopTracker. Those will probably use an external test script that waits for e.g. a specific print from Lua.

Tests should be run on both 64bit and 32bit to see if any integer size assumptions are wrong.

Tests that should be done:
- Render tests for all example packs
- High level tests for AP
  - 64bit onItem
  - 64bit onLocationScout
  - 64bit onLocationCheck
  - 64bit CheckedLocations
  - 64bit MissingLocations
  - 64bit data storage
  - unicode data storage
  - 64bit slot data
  - unicode slot data

## How to add Unit Tests

We compile `test/*/*.cpp` together with googletest and `src/**` into a test binary. To add tests, simply pick a folder
and add a `test_*.cpp` file that uses googletest API.

## How to run Unit Tests

* install `googltest`, sometimes called `gtest` or `libgtest-dev` and `libgmock-dev`, via pacman, apt, brew, etc. or
  from source
* `make test`

## Other Tests

We have not decided on a solution for non unit tests yet.
