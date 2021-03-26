#ifndef _CORE_IMAGEREFERENCE_H
#define _CORE_IMAGEREFERENCE_H

#include "../luaglue/luainterface.h"


class ImageReference : public LuaInterface<ImageReference> {
    friend class LuaInterface;
    
public:
    const char* FromPackRelativePath(const char* path) const;
    
protected: // LUA interface implementation
    static constexpr const char Lua_Name[] = "ImageReference";
    static const LuaInterface::MethodMap Lua_Methods;
};

#endif /* _CORE_IMAGEREFERENCE_H */

