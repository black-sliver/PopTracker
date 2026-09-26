# PopTracker Fuzz Testing

We have a single fuzz target that fuzzes all things we have fuzzers for.

## Building the fuzz target

Only clang is supported at the moment and provides coverage guided fuzzing.

The fuzz target can be built using
```sh
cd ..
CC=clang CXX=clang++ meson setup build-fuzzer -Dauto-lto=false --buildtype debugoptimized -Doptimization=g -Db_sanitize=address,undefined
meson compile -C build-fuzzer fuzzer
```

**IMPORTANT**: build flags between "normal" builds and the fuzz target may be incompatible, e.g. through `liblua`.
We use a separate build dir (`build-fuzzer` instead of `build`) for that reason.

## Setting up a corpus

Saving the corpus (test data) makes it so that continuing already can reach more code paths.
Additionally, the corpus can be initialized from known data.

```sh
cd ..
mkdir corpus
python test/core/tools/make_zip.py
./build-fuzzer/test/fuzzer -merge=1 corpus test/core/data
```

## Running the fuzzer

```sh
cd ..
./build-fuzzer/test/fuzzer corpus  # 1 process that prints to stdout
# or 
# ./build-fuzzer/test/fuzzer corpus -jobs=4 # 4 processes that print to fuzz-{0-3}.log
```

See [libFuzzer documentation](https://llvm.org/docs/LibFuzzer.html#options) for more options.

## Compressing a corpus

While fuzzing, the corpus will grow with possibly-uninteresting data.
It can be reduced to data that is actually interesting by merging the old corpus into an empty one:
```sh
cd ..
mv corpus old-corpus
mkdir corpus
./build-fuzzer/test/fuzzer -merge=1 corpus old-corpus && rm -r old-corpus
```

## Writing a fuzz test

There is a `FUZZ` macro in `test/fuzz.hpp`, that allows adding a function to the fuzzer.
See `test/core/test_zip.cpp`.

## Fuzzing during unit tests

TODO: in addition, we also want to do minimal (non-coverage-guided) fuzzing while running unit tests.

## Fuzzing in CI

See `.github/workflows/fuzz.yaml`.

## Recommended Build Options

### UBSAN

Undefined behavior could completely invalidate the tests.
While UBSAN adds a lot of overhead, it feels like the sane choice.

This is currently hard-coded in `test/meson.build` as well.

### Debug Build

Debug enables some assertions that could lead to a crash.

### Optimization Level 2

This is what release builds are shipped with, and it may be faster than `-Og`.
Fuzzer crashes will be recorded and can then be inspected `-Og` or `-O0`.

### No LTO

Coverage currently does not work when using LTO with clang.
Disable LTO with `-Dauto-lto=false` and not manually enabling it.
