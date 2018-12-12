// Copyright (c) 2018 Monacoinprojct

#ifndef BITCOIN_PLUGIN_H
#define BITCOIN_PLUGIN_H

#include <lua/lua/lua.hpp>
#include <lua/lua/lauxlib.h>
#include <lua/lua/lualib.h>
#include <sync.h>
#include <univalue.h>

#include <string>
#include <deque>

namespace plugin {

//============================================================================

//============================================================================

void Init();
void Term();

bool LoadPlugin(const char *filename);
bool UnloadPlugin(const char *filename);

void WalletNotify(const std::string &hash);
void BlockNotify(bool initialsync, const std::string &hash);

//============================================================================
// class CPlugin
//============================================================================

class CPlugin
{
public:
    CPlugin();
    ~CPlugin();

    bool Load(const char* filename);
    void Unload();

    void WalletNotify(std::string hash);
    void BlockNotify(bool initialsync, std::string hash);

private:
    lua_State *L;
};

};

#endif // BITCOIN_PLUGIN_H
