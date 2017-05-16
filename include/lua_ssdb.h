#ifndef HENETWORKS_SSDB
#define HENETWORKS_SSDB

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "lua51.lib")

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <lua_helper.h>
}

#define DLUASSDBNAME "ssdb"
#define DLUASSDBMETA ":ssdbmeta:"

EXTERNC int luaopen_ssdb(lua_State *l);

#endif