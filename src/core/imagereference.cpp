#include "imagereference.h"

#include <luaglue/luamethod.h>

const LuaInterface<ImageReference>::MethodMap ImageReference::Lua_Methods = {
    LUA_METHOD(ImageReference, FromPackRelativePath, const char*),
};

const char* ImageReference::FromPackRelativePath(const char* path) const { return path; }