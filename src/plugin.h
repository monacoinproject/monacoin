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

bool LoadPluginCode(const char *sourcecode);
bool UnloadPluginCode();

void WalletNotify(const std::string &hash);
void BlockNotify(bool initialsync, const std::string &hash);

};

#endif // BITCOIN_PLUGIN_H
