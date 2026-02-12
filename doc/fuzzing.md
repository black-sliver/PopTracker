# PopTracker Fuzz Testing

We have a single fuzz target that fuzzes all things we have fuzzers for.

## Building the fuzz target

Only clang is supported at the moment and provides coverage guided fuzzing.

The fuzz target can be built using
```sh
cd ..
CC=clang CXX=clang++ make fuzzer CONF=DEBUG WITH_UBSAN=1 WITH_ASAN=1 WITH_FUZZER=1 -j 8
```

**IMPORTANT**: build flags between "normal" builds and the fuzz target are incompatible, and they currently share the
same intermediate paths, so `make clean` is required when switching between fuzzing and testing/running!

## Setting up a corpus

Saving the corpus (test data) makes it so that continuing already can reach more code paths.
Additionally, the corpus can be initialized from known data.

```sh
cd ..
mkdir corpus
python test/core/tools/make_zip.py
./build/*/fuzzer -merge=1 corpus test/core/data
```

## Running the fuzzer

```sh
cd ..
./build/*/fuzzer corpus  # 1 process that prints to stdout
# or 
# ./build/*/fuzzer corpus -jobs=4 # 4 processes that print to fuzz-{0-3}.log
```

## Compressing a corpus

While fuzzing, the corpus will grow with possibly-uninteresting data.
It can be reduced to data that is actually interesting by merging the old corpus into an empty one:
```sh
cd ..
mv corpus old-corpus
mkdir corpus
./build/*/fuzzer -merge=1 corpus old-corpus && rm -r old-corpus
```

## Writing a fuzz test

There is a `FUZZ` macro in `test/fuzz.hpp`, that allows adding a function to the fuzzer.
See `test/core/test_zip.cpp`.

## Fuzzing during unit tests

TODO: in addition, we also want to do minimal (non-coverage-guided) fuzzing while running unit tests.

## Fuzzing in CI

TODO: we want to run the fuzzer with cached corpus in CI.
