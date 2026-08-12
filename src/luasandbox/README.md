# Lua Sandbox

Sandboxed variants of some common Lua modules and functions.

## Implementation Details

Native objects may be stored as light userdata pointers in `LUA_REGISTRYINDEX`, which is inaccessible from within Lua.
Externally-owned native objects have to outlive the Lua State, otherwise a dangling pointer could be used during runtime
or GC.

## Features

### require

Implements a minimal `require` function that is restricted to the pack.

A `Pack*` light userdata object needs to be available as `"Pack"` in `LUA_REGISTRYINDEX`.

#### Usage

```cpp
lua_pushstring(L, "Pack");
lua_pushlightuserdata(L, (void*)pack);
lua_settable(L, LUA_REGISTRYINDEX);
lua_pushcfunction(L, luasandbox_require);
lua_setglobal(L, "require");
```
```lua
require "module_inside_pack"
```

### LuaPackIO (io)

Implements a minimal `io` module that is restricted to the pack.

#### Usage

```cpp
luaio = new LuaPackIO(pack);
LuaPackIO::Lua_Register(L);
LuaPackIO::File::Lua_Register(L);
luaio->Lua_Push(L);
lua_setglobal(L, LUA_IOLIBNAME);
```
```lua
local file = io.open("relative/to/pack")
local data = file.read()
```
