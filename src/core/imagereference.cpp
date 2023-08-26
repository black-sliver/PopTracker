#include "imagereference.h"

#include <luaglue/luamethod.h>

const LuaInterface<ImageReference>::MethodMap ImageReference::Lua_Methods = {
    LUA_METHOD(ImageReference, FromPackRelativePath, const char*),
    LUA_METHOD(ImageReference, FromImageReference, const char*, const char*),
};

const char* ImageReference::FromPackRelativePath(const char* path) const
{
    return path;
}

std::string ImageReference::FromImageReference(const char* orig, const char* mod) const
{
    // for now ImageReference is fake and we just concatenate path and mods
    std::string s = orig;
    auto pos = s.find(':');
    if (pos != s.npos) {
        return std::string(orig) + "," + mod;
    } else {
        return std::string(orig) + ":" + mod;
    }
}
