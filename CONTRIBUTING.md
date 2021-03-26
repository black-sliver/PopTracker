# Contributing to PopTracker

- check [doc/OUTLINE.md](doc/OUTLINE.md) to get an idea how this is supposed to work,
- check [doc/TODO.md](doc/TODO.md) and `//TODO:` comments to see what has to be done.

Send PRs on github.

## Coding style
- cpp and h filenames are all lowercase, named after the class name
- include guards are all upper case, start with _ and end with _H
- CamelCase
- public member variables start with a captial letter
- protected and private member variables start with _
- local variables start with a lower case letter
- public methods start with a lower case letter? *Up to debate*
- protected methods start with a lower case letter? *Up to debate*
- getters start with get, setters with set
- class names start with a captial letter
- one exception to the above is the Lua Inferface, which uses T::Lua_CamelCase
- loops/iterations use auto : or auto& : where possible
- avoid `std::stream`
- use `std::string` or `std::string_view` (we target c++17)
- stuff is open to debate. PRs for coding style will be welcome at some point.

## Ownership model
- ownership should be similar to Qt, where a parent will delete its children
- we will probably mostly use "non-smart" pointers, similar to Qt
- prefer clear ownership and life cycle over smart pointers

## Optimizations
- since we want to deploy to wasm, try to use as little of libstdc++ as possible without impacting maintainability

