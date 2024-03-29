# Contributing to PopTracker

- check [doc/OUTLINE.md](doc/OUTLINE.md) to get an idea how this is supposed to work,
- check [doc/TODO.md](doc/TODO.md) and `//TODO:` comments to see what has to be done.

Send PRs on github.

## C++ Style

- cpp and h filenames are all lowercase, named after the class name
- 120 chars per line
- new code should mostly follow [WebKit C++ style](https://webkit.org/code-style-guidelines/),
  with exception of
  - `m_` for member variables is not required: just `_` is fine (read below)
  - `s_` for static members is not required
  - getters start with `get` - it would take a major refactor to change them all
- include guards are `_FOLDER_FOLDER_FILENAME_H`,
  closing `#endif` should have the name as comment
- `#pragma once` is preferred for new code
- camelCase
- protected and private member variables start with `_`
- local variables start with a lower case letter
- public methods start with a lower case letter
- protected methods start with a lower case letter
- public member variables should start with a lower case letter - this is to match SDL's structs
- public member variables should only be used for simple structs - use getters/setters otherwise
- getters start with `get`, setters with `set`
- class names start with a captial letter
- one exception to the above is the Lua Inferface, which uses `T::Lua_CamelCase`
- loops/iterations use `auto :` or `auto& :` where possible
- use `std::string` or `std::string_view` (we target c++17)
- there are a ton of violations, but new code should still try to check all the boxes
- individual modules/folders/files can have their own style, which should be followed
- stuff is open to debate. PRs for coding style will be welcome at some point.

### Ownership model

- ownership should be similar to Qt, where a parent will delete its children
- unique_ptr are fine, "raw pointers" are also fine, but should probably refactor uilib at some point
- avoid shared and weak since they are far from free; prefer clear ownership and life cycle

### "Mandatory" Optimizations

- we want to be able to deploy to wasm
  - total code size is relevant for loading times / download size
  - wasm is a lot slower than native code
  - memory allocation in multiple threads is slow
- try to use as little of libstdc++ as possible without impacting maintainability
  - dead code in static builds gets stripped and the complete libstdc++ is massive
  - calling directly into libc is often cheaper (printf, fread vs. std::streams)
- try to avoid exceptions and RTTI - some libs depend on exceptions,
  so we don't aim for `-fno-exceptions` at the moment
- try to write fast code by default
- assume threads are not cheap
- use private (not protected) where possible to avoid going through vtables
- implement small functions inline in header files for platforms where LTO does not work correctly

## Documentation Style

Markdown per [Google Markdown style guide](https://google.github.io/styleguide/docguide/style.html)
with the following exceptions:
- single space after bullet / list number
- no lazy numbering because that's awful to read in source form
- nesting lists with two spaces

120 chars per line.

All supported features should be documented in doc/ and added to the json schema in schema/.

Most files have not been updated to follow the style fully. When touching a section, the code style of that section
should be fixed.
