#include "luapackio.h"
#include "lstate.h"

const LuaInterface<LuaPackIO>::MethodMap LuaPackIO::Lua_Methods = {};
const LuaInterface<LuaPackIO::File>::MethodMap LuaPackIO::File::Lua_Methods = {};

LuaPackIO::LuaPackIO(const Pack* pack)
    : _pack(pack), _input(nullptr)
{
}

LuaPackIO::~LuaPackIO()
{
    _pack = nullptr;
    _input = nullptr;
}

void LuaPackIO::Lua_GC(lua_State* L)
{
    if (_input) {
        luaL_unref(L, LUA_REGISTRYINDEX, _inputRef.ref);
        _input = nullptr;
    }
}

int LuaPackIO::Lua_Index(lua_State* L, const char* key)
{
    if (strcmp(key, "open") == 0) {
        Lua_Push(L);
        lua_pushcclosure(L, [](lua_State* L) -> int {
            return luaL_checkthis(L, lua_upvalueindex(1))->open(L);
        }, 1);
    }
    else if (strcmp(key, "input") == 0) {
        Lua_Push(L);
        lua_pushcclosure(L, [](lua_State* L) -> int {
            return luaL_checkthis(L, lua_upvalueindex(1))->input(L);
        }, 1);
    }
    else if (strcmp(key, "write") == 0) {
        Lua_Push(L);
        lua_pushcclosure(L, [](lua_State* L) -> int {
            // write is not supported
            luaL_error(L, "Write not supported");
            return 0;
        }, 1);
    }
    else if (strcmp(key, "read") == 0) {
        Lua_Push(L);
        lua_pushcclosure(L, [](lua_State* L) -> int {
            auto self = luaL_checkthis(L, lua_upvalueindex(1));
            if (!self->_input) {
                luaL_error(L, "No input file");
                return 0;
            }
            return self->_input->read(L);
        }, 1);
    }
    else if (strcmp(key, "lines") == 0) {
        Lua_Push(L);
        lua_pushcclosure(L, [](lua_State* L) -> int {
            auto self = luaL_checkthis(L, lua_upvalueindex(1));
            if (!self->_input) {
                luaL_error(L, "No input file");
                return 0;
            }
            // need ref to input for closure
            lua_rawgeti(L, LUA_REGISTRYINDEX, self->_inputRef.ref);
            lua_insert(L, 1);
            // run code
            int res = self->_input->lines(L, 2);
            // remove ref again
            lua_remove(L, 1);
            return res;
        }, 1);
    }
    else if (strcmp(key, "seek") == 0) {
        Lua_Push(L);
        lua_pushcclosure(L, [](lua_State* L) -> int {
            auto self = luaL_checkthis(L, lua_upvalueindex(1));
            if (!self->_input) {
                luaL_error(L, "No input file");
                return 0;
            }
            return self->_input->seek(L);
        }, 1);
    }
    else {
        return 0;
    }
    return 1;
}

int LuaPackIO::open(lua_State* L)
{
    const char* filename = luaL_checkstring(L, 1);
    const char* mode = luaL_optstring(L, 2, "r");
    if (!mode || (mode[0] != 'r')) {
        luaL_error(L, "Only read supported");
        return 0;
    }
    std::string buf;
    if (!_pack->ReadFile(filename, buf)) {
        lua_pushnil(L);
        lua_pushstring(L, "No such file or directory");
        lua_pushinteger(L, ENOENT);
        return 3;
    }
    File* f = new File(buf);
    f->Lua_Push(L);
    return 1;
}

int LuaPackIO::input(lua_State* L)
{
    if (_input && (lua_gettop(L) == 0 || lua_isnil(L, -1))) {
        // NOTE: LuaInterface::Lua_Push can't be used with __gc without addiional ref count
        lua_rawgeti(L, LUA_REGISTRYINDEX, _inputRef.ref);
        luaL_unref(L, LUA_REGISTRYINDEX, _inputRef.ref);
        _input = nullptr;
        _inputRef.ref = LUA_NOREF;
        return 1;
    }
    if (lua_gettop(L) == 0 || lua_isnil(L, -1)) {
        lua_pushnil(L);
        return 1;
    }
    if (_input) {
        luaL_unref(L, LUA_REGISTRYINDEX, _inputRef.ref);
        _input = nullptr;
        _inputRef.ref = LUA_NOREF;
    }
    _input = File::luaL_testthis(L, 1);
    if (!_input) {
        const char* fn = luaL_checkstring(L, 1);
        {
            std::string buf;
            if (_pack->ReadFile(fn, buf))
                _input = new File(buf);
        }
    }
    if (!_input) {
        luaL_error(L, "Could not open file");
    } else {
        _inputRef.ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    return 0;
}


LuaPackIO::File::File(const std::string& data)
    : _data(data), _pointer(0), _open(true)
{
}

LuaPackIO::File::~File()
{
    close();
}

void LuaPackIO::File::Lua_GC(lua_State* L)
{
    delete this;
}

int LuaPackIO::File::Lua_Index(lua_State* L, const char* key)
{
    if (strcmp(key, "read") == 0) {
        lua_pushcfunction(L, [](lua_State* L) -> int {
            auto self = luaL_checkthis(L, 1);
            if (!self) return 0;
            return self->read(L, 2);
        });
    }
    else if (strcmp(key, "write") == 0) {
        lua_pushcfunction(L, [](lua_State* L) -> int {
            auto self = luaL_checkthis(L, 1);
            if (!self) return 0;
            return self->write(L, 2);
        });
    }
    else if (strcmp(key, "seek") == 0) {
        lua_pushcfunction(L, [](lua_State* L) -> int {
            auto self = luaL_checkthis(L, 1);
            if (!self) return 0;
            return self->seek(L, 2);
        });
    }
    else if (strcmp(key, "lines") == 0) {
        lua_pushcfunction(L, [](lua_State* L) -> int {
            auto self = luaL_checkthis(L, 1);
            if (!self) return 0;
            return self->lines(L, 2);
        });
    }
    else if (strcmp(key, "close") == 0) {
        lua_pushcfunction(L, [](lua_State* L) -> int {
            auto self = luaL_checkthis(L, 1);
            if (!self) return 0;
            return self->close();
        });
    }
    else {
        return 0;
    }
    return 1;
}

int LuaPackIO::File::close(lua_State* L)
{
    if (_open) {
        if (L) {
            lua_pushboolean(L, true);
        }
        _data.clear();
        _pointer = 0;
        _open = false;
        return 1;
    }
    if (L) {
        luaL_error(L, "attempt to use a closed file");
    }
    return 0;
}

int LuaPackIO::File::read(lua_State* L, const char* mode)
{
    if (mode == nullptr || strcmp(mode, "*line") == 0) {
        if (_pointer >= _data.length()) {
            lua_pushnil(L);
        } else {
            auto cr = _data.find('\r', _pointer);
            auto lf = _data.find('\n', _pointer);
            auto end = (cr == _data.npos) ? lf : (lf == _data.npos) ? cr : std::min(cr, lf);
            lua_pushlstring(L, _data.c_str()+_pointer, end-_pointer);
            _pointer = end;
            if (_pointer < _data.length() && _data[_pointer] == '\n') {
                _pointer++;
            } else if (_pointer < _data.length() && _data[_pointer] == '\r') {
                _pointer++;
                if (_pointer < _data.length() && _data[_pointer] == '\n') {
                    _pointer++;
                }
            }
        }
    } else if (strcmp(mode, "*all") == 0) {
        size_t off = std::min(_data.length(), _pointer);
        lua_pushlstring(L, _data.c_str()+off, _data.length()-off);
    } else if (strcmp(mode, "*number") == 0) {
        if (_pointer >= _data.length()) {
            lua_pushnil(L);
        } else {
            const char* s = _data.c_str() + _pointer;
            char* next = NULL;
            double val = strtod(s, &next);
            if (next && next != s) {
                _pointer = next - _data.c_str();
                lua_pushnumber(L, val);
            } else {
                lua_pushnil(L);
            }
        }
    } else {
        luaL_error(L, "unsupported read mode: %s", mode);
    }
    return 1;
}

int LuaPackIO::File::read_bytes(lua_State* L, size_t bytes)
{
    if (_pointer >= _data.length()) {
        lua_pushnil(L);
        return 1;
    }
    if (_pointer + bytes > _data.length()) {
        bytes = _data.length() - _pointer;
    }
    lua_pushlstring(L, _data.c_str()+_pointer, bytes);
    _pointer += bytes;
    return 1;
}

int LuaPackIO::File::read(lua_State* L, int n)
{
    if (!_open) {
        luaL_error(L, "attempt to use a closed file");
        return 0;
    }
    int res = 0;
    int top = lua_gettop(L);
    for (int i=n; i<=top; i++) {
        if (lua_isinteger(L, i)) {
            res += read_bytes(L, lua_tointeger(L, i));
        } else if (lua_isstring(L, i)) {
            res += read(L, lua_tostring(L, i));
        } else {
            luaL_error(L, "bad argument #%d to read", i);
        }
    }
    return res;
}

int LuaPackIO::File::write(lua_State* L, int n)
{
    if (!_open) {
        luaL_error(L, "attempt to use a closed file");
        return 0;
    }
    lua_pushnil(L);
    lua_pushstring(L, "Write not supported");
    lua_pushinteger(L, -1);
    return 3;
}

int LuaPackIO::File::seek(lua_State* L, int n)
{
    if (!_open) {
        luaL_error(L, "attempt to use a closed file");
        return 0;
    }
    const char* whence = luaL_optstring(L, n, "cur");
    lua_Integer offset = luaL_optinteger(L, n+1, 0);
    if (strcmp(whence, "") == 0 || strcmp(whence, "cur") == 0) {
        _pointer += offset;
    } else if (strcmp(whence, "set") == 0) {
        _pointer = offset;
    } else if (strcmp(whence, "end") == 0) {
        _pointer = _data.length() + offset;
    } else {
        luaL_error(L, "bad argument #%d to 'seek' ('cur', 'set' or 'end' expected)", n);
        return 0;
    }
    if (_pointer <= std::numeric_limits<LUA_INTEGER>::max())
        lua_pushinteger(L, (LUA_INTEGER)_pointer);
    else
        lua_pushnumber(L, (LUA_NUMBER)_pointer);
    return 1;
}

int LuaPackIO::File::lines(lua_State* L, int n)
{
    if (!_open) {
        luaL_error(L, "attempt to use a closed file");
        return 0;
    }
    if (n>1 && luaL_testthis(L, n-1)) {
        lua_pushvalue(L, n-1);
    } else {
        luaL_error(L, "can't capture File");
        return 0;
    }
    lua_pushcclosure(L, [](lua_State* L) -> int {
        auto self = luaL_checkthis(L, lua_upvalueindex(1));
        return self->read(L, nullptr);
    }, 1);
    return 1;
}
