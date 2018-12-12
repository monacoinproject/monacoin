// Copyright (c) 2018 Monacoinprojct

#ifndef _LUAUSER_H_
#define _LUAUSER_H_

#define lua_lock(L) LuaLock(L)
#define lua_unlock(L) LuaUnlock(L)
#define luai_userstateclose(L) LuaUnlock(L)

#ifdef __cplusplus
extern "C" {
#endif

void LuaLock(lua_State *L);
void LuaUnlock(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif // _LUAUSER_H_
