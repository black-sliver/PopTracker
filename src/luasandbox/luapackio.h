#ifndef _LUASANDBOX_LUAPACKIO_H
#define _LUASANDBOX_LUAPACKIO_H

#include <luaglue/luainterface.h>
#include "../core/pack.h"

class LuaPackIO  final : public LuaInterface<LuaPackIO> {
    friend class LuaInterface;

public:
    LuaPackIO(const Pack* pack);
    virtual ~LuaPackIO();

    class File final : public LuaInterface<LuaPackIO::File> {
        friend class LuaInterface;
        friend class LuaPackIO;

    public:
        virtual ~File();

    protected:
        File(const std::string& data);
        std::string _data;
        size_t _pointer;
        bool _open;

        int read(lua_State* L, const char* mode);
        int read_bytes(lua_State* L, size_t bytes);
        int read(lua_State* L, int n=1);
        int write(lua_State* L, int n=1);
        int seek(lua_State* L, int n=1);
        int lines(lua_State* L, int n=1);
        int close(lua_State* L=nullptr);

    protected: // Lua interface implementation
        static constexpr const char Lua_Name[] = "LuaPackIO.File";
        static const LuaInterface::MethodMap Lua_Methods;

        virtual int Lua_Index(lua_State* L, const char* key) override;
        virtual void Lua_GC(lua_State* L) override;
    };


protected:
    const Pack* _pack;
    File* _input;
    LuaRef _inputRef;

    int open(lua_State* L);
    int input(lua_State* L);

protected: // Lua interface implementation
    static constexpr const char Lua_Name[] = "LuaPackIO";
    static const LuaInterface::MethodMap Lua_Methods;

    virtual int Lua_Index(lua_State* L, const char* key) override;
    virtual void Lua_GC(lua_State* L) override;
};

#endif /* _LUASANDBOX_LUAPACKIO_H */
