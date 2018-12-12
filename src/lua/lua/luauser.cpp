// Copyright (c) 2018 Monacoinprojct

#include "lua.h"
#include "luauser.h"

#include <mutex>

//============================================================================
// local work
//============================================================================

static std::mutex mtx;

//============================================================================
// global func
//============================================================================

void LuaLock(lua_State *L)
{
    mtx.lock();
}

void LuaUnlock(lua_State *L)
{
    mtx.unlock();
}
