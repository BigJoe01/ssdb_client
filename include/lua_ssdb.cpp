/*
MIT License

Copyright( c ) 2017 Joe Oszlanczi rawengroup@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files( the "Software" ), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "lua_ssdb.h"
#include <assert.h>
#include "SSDB_client.h"
#include <string>
#include <vector>
#include <map>
#include <exception>

#if defined(_WIN32)

#define DNEED_WSA_STARTUP
	#if defined(DNEED_WSA_STARTUP)
		#define WIN32_LEAN_AND_MEAN
		#include <WinSock2.h>
	#endif
#endif

/***

Lua ssdb c module.

@author Joe Oszlanczi rawengroup@gmail.com
@copyright 2017
@license MIT/X11
@module ssdb
*/

#pragma warning ( disable : 4244 ) // conversion from 'lua_Number' to 'int64_t',
#pragma warning ( disable : 4267 ) // conversion from 'size_t' to 'int'

/// Internal, check userdata type and convert to ssdb client
// @function SSDB_CHECK
// @param l lua_state
// @param int iIndex
// @return ssdb::client
ssdb::Client* SSDB_CHECK( lua_State* l, int iIndex )
{
	#ifdef _DEBUG
		assert( lua_gettop( l ) > 0 );
		ssdb::Client* instance = *( ssdb::Client** )luaL_checkudata( l, iIndex, DLUASSDBMETA );
		assert( instance != nullptr );
		if ( !instance )
			luaL_error( l, "Invalid SSDB meta at index : %d", iIndex );
		return instance;
	#else
		return  *( ssdb::Client** )lua_touserdata( l, iIndex );
	#endif
}

/// Internal, create new ssdb client and userdata
// @function SSDB_NEW
// @param l lua_state
// @param host string host address
// @param port connection port number
// @return ssdb::client
ssdb::Client* SSDB_NEW( lua_State* l, const char* sHost, int iPort )
{
	ssdb::Client** ppStream = ( ssdb::Client** )lua_newuserdata( l, sizeof( ssdb::Client* ) );
	if ( sHost && iPort != 0 )
	{
		try
		{
			*ppStream = ssdb::Client::connect( sHost, iPort );
		}
		catch( std::exception e )
		{
			*ppStream = NULL;
		}
	}
	else
		ppStream = NULL;
	luaL_getmetatable( l, DLUASSDBMETA );
	lua_setmetatable( l, -2 );
	return *ppStream;
}

/// internal, convert vector to lua table
// @function conver_vector_to_table
// @param array string vector
// @param l lua_state
inline int convert_vector_to_table( const std::vector<std::string>& aList, lua_State* l )
{
	int iRes = 0;
	lua_createtable( l, aList.size(), 0 );
	if ( !aList.empty() )
	{
		lua_pushvalue( l, -1 );
		for ( int iIndex = 0; iIndex < aList.size(); iIndex++ )
		{
			lua_pushstring( l, aList[ iIndex ].c_str() );
			lua_rawseti( l, -2, iIndex + 1 );
			iRes++;
		}
		lua_pop( l, 1 );
	}
	return iRes;
}

/// internal, convert vector stored map to lua table
// @function conver_vectormap_to_table
// @param array string vector, contains map
// @param l lua_state
inline int convert_vectormap_to_table( const std::vector<std::string>& aList, lua_State* l, bool bNumber = false )
{
	int iRes = 0;
	lua_createtable( l, 0, aList.size() / 2 );
	if ( !aList.empty() )
	{
		lua_pushvalue( l, -1 );
		int iIndex = 0;
		while ( iIndex < aList.size() )
		{
			lua_pushstring( l, aList[ iIndex ].c_str() );
			iIndex++;
			if ( bNumber )
				lua_pushnumber( l, std::atoll( aList[ iIndex ].c_str()) );
			else
				lua_pushstring( l, aList[ iIndex ].c_str() );
			lua_settable( l, -3 );
			iRes++;
		}
		lua_pop( l, 1 );
	}
	return iRes;
}

/// internal, convert lua table to array list
// @function conver_vectormap_to_table
// @param array string vector, contains map
// @param l lua_state
inline int convert_lua_table_vector( std::vector<std::string>& aList, lua_State* l, int iIndex )
{
	int iRes = 0;
	lua_pushvalue( l, iIndex );
	lua_pushnil( l );
	while ( lua_next( l, -2 ) )
	{
		if ( lua_type( l, -1 ) == LUA_TSTRING )
		{
			aList.push_back( lua_tostring( l, -1 ) );
			iRes++;
		}
		lua_pop( l, 1 );
	}
	lua_pop( l, 1 );
	return iRes;
}

/// internal, convert lua table to map list
// @function convert_lua_table_map
// @param map contains string map
// @param l lua_state
inline int convert_lua_table_map( std::map<std::string,std::string>& aMap, lua_State* l, int iIndex )
{
	int iRes = 0;
	lua_pushvalue( l, iIndex );
	lua_pushnil( l );
	while ( lua_next( l, -2 ) )
	{
		if ( lua_type( l, -2 ) == LUA_TNIL )
		{
			aMap[ lua_tostring( l, -2 ) ] = lua_tostring( l, -1 );
			iRes++;
		}
		lua_pop( l, 1 );
	}
	lua_pop( l, 1 );
	return iRes;
}

/// internal, convert lua table to map list
// @function convert_lua_table_map
// @param map contains string map
// @param l lua_state
inline int convert_lua_table_map( std::map<std::string, int64_t>& aMap, lua_State* l, int iIndex )
{
	int iRes = 0;
	lua_pushvalue( l, iIndex );
	lua_pushnil( l );
	while ( lua_next( l, -2 ) )
	{
		if ( lua_type( l, -2 ) == LUA_TNIL )
		{
			aMap[ lua_tostring( l, -2 ) ] = lua_tonumber( l, -1 );
			iRes++;
		}
		lua_pop( l, 1 );
	}
	lua_pop( l, 1 );
	return iRes;
}

/// create new ssdb client
// @function connect
// @param ip remote address
// @param port remote port
// @return client
// @return bool connection success to server
int ssdb_connect( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 1 ) == LUA_TSTRING && lua_type( l, 2 ) == LUA_TNUMBER )
	ssdb::Client* pClient = SSDB_NEW( l, lua_tostring( l, 1 ), lua_tointeger( l, 2 ) );
	lua_pushboolean( l, pClient ? true : false );
	return 2;
}

/// destroy client instance
// @function __gc
// @param instance ssdb::Client
int ssdb_client_gc( lua_State* l )
{
	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	if ( pClient )
		delete pClient;
	return 0;
}

/// generate error status for lua result
// @function error_status
// @param status ssdb::Status
// @return bool true is success
// @return string error type string [ optional ] 'connection', 'notfound', 'unknown'
// @return string error code string [ optional ]
bool error_status( lua_State* l, ssdb::Status& Status )
{
	if ( Status.ok() )
	{
		lua_pushboolean( l, true );
		return false;
	}
	else
	{
		lua_pushboolean( l, false );
		if ( Status.error() )
			lua_pushstring(l, "connection" );
		else if ( Status.not_found() )
			lua_pushstring(l, "notfound" );
		else
			lua_pushstring(l, "unknown" );
		lua_pushstring( l, Status.code().c_str() );
		return true;
	}
}

/// get database size
// @function dbsize
// @param instance ssdb::client
// @return success true is success
// @return number current database size
int ssdb_client_dbsize( lua_State* l )
{
	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	int64_t i64Size;
	if ( error_status( l, pClient->dbsize( &i64Size ) ))
		return 3;
	else
	{
		lua_pushnumber( l, i64Size );
		return 2;
	}
}

/// get kv range
// @function get_kv_range
// @param instance ssdb::client
// @return bool true is success
// @return string start range or empty string
// @return string end range or empty string
// @see error_status
int ssdb_client_get_kv_range( lua_State* l )
{
	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKvStart, sKvEnd;
	if ( error_status( l, pClient->get_kv_range( &sKvStart, &sKvEnd) ) )
		return 3;
	else
	{
		lua_pushstring( l, sKvStart.c_str() );
		lua_pushstring( l, sKvEnd.c_str() );
		return 3;
	}
}

/// set kv range
// @function set_kv_range
// @param instance ssdb::client
// @param string kv start range
// @param string kv end range
// @return bool true is success
// @see error_status
int ssdb_client_set_kv_range( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TSTRING );
	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKvStart = lua_tostring( l, 2 );
	std::string sKvEnd = lua_tostring( l, 3 );
	if ( error_status( l, pClient->set_kv_range( sKvStart, sKvEnd ) ) )
		return 3;
	else
		return 1;
}

/// set key/value
// @function set
// @param instance ssdb::client
// @param string name
// @param string value
// @return bool true is success
// @see error_status
int ssdb_client_set_kv( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 && ( lua_type( l, 2 ) == LUA_TSTRING || lua_type( l, 2 ) == LUA_TNUMBER) && lua_type( l, 3 ) == LUA_TSTRING );
	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sName = lua_tostring( l, 2 );
	std::string sValue = lua_tostring( l, 3 );
	if ( error_status( l, pClient->set( sName, sValue ) ) )
		return 3;
	else
		return 1;
}

/// get key value
// @function get
// @param instance ssdb::client
// @param string name
// @return bool true is success
// @return string value
// @see error_status
int ssdb_client_get_kv( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 2 ) == LUA_TSTRING );
	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sName = lua_tostring( l, 2 );
	std::string sValue;
	if ( error_status( l, pClient->get( sName, &sValue ) ) )
		return 3;
	else
	{
		lua_pushstring( l, sValue.c_str() );
		return 2;
	}
}

/// set key/value with ttl
// @function set_ttl
// @param instance ssdb::client
// @param string name
// @param string value
// @param number ttl
// @return bool true is success
// @see error_status
int ssdb_client_set_kv_ttl( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 3 && 
		lua_type( l, 2 ) == LUA_TSTRING && 
		lua_type( l, 3 ) == LUA_TSTRING &&
		lua_type( l, 4 ) == LUA_TNUMBER );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sName     = lua_tostring( l, 2 );
	std::string sValue    = lua_tostring( l, 3 );
	int iTTL              = lua_tointeger( l, 4 );

	if ( error_status( l, pClient->setx( sName, sValue, iTTL ) ) )
		return 3;
	else
		return 1;
}

/// del key
// @function del
// @param instance ssdb::client
// @param string name
// @return bool true is success
// @see error_status
int ssdb_client_del_kv( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 2 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sName = lua_tostring( l, 2 );
	if ( error_status( l, pClient->del( sName ) ) )
		return 3;
	else
		return 1;
}

/// increment key value by value
// @function inc
// @param instance ssdb::client
// @param string name
// @param number incby
// @return bool true is success
// @return number new value
// @see error_status
int ssdb_client_inc_kv( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TNUMBER );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sName = lua_tostring( l, 2 );
	int64_t iInc = lua_tonumber( l, 3 );
	int64_t iNewValue;
	if ( error_status( l, pClient->incr( sName, iInc, &iNewValue ) ) )
		return 3;
	else
	{
		lua_pushnumber( l, iNewValue );
		return 2;
	}
}

/// get keys from range, with limit
// @function keys
// @param instance ssdb::client
// @param strig key_start empty string or starter value
// @param string key_end empty string or end value
// @param number limit key limit size
// @return bool true is success
// @return table with key values
// @see error_status
int ssdb_client_keys( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 3 && 
					lua_type( l, 2 ) == LUA_TSTRING && 
					lua_type( l, 3 ) == LUA_TSTRING && 
					lua_type( l, 4 ) == LUA_TNUMBER );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sNameStart = lua_tostring( l, 2 );
	std::string sNameEnd = lua_tostring( l, 3 );
	uint64_t iLimit = lua_tonumber( l, 4 );
	std::vector<std::string> vResult;
	if ( error_status( l, pClient->keys( sNameStart, sNameEnd, iLimit, &vResult) ) )
		return 3;
	else
	{
		convert_vector_to_table( vResult, l );
		return 2;
	}
}

///scan get names and values from range with limit
// @function scan
// @param instance ssdb::client
// @param string start key or empty string
// @param string end key or empty string
// @param number key limit size
// @return bool true is success
// @return table with keys and values
// @see error_status
int ssdb_client_scan( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 3 &&
		lua_type( l, 2 ) == LUA_TSTRING &&
		lua_type( l, 3 ) == LUA_TSTRING &&
		lua_type( l, 4 ) == LUA_TNUMBER );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sNameStart = lua_tostring( l, 2 );
	std::string sNameEnd = lua_tostring( l, 3 );
	uint64_t iLimit = lua_tonumber( l, 4 );
	std::vector<std::string> vResult;
	if ( error_status( l, pClient->scan( sNameStart, sNameEnd, iLimit, &vResult ) ) )
		return 3;
	else
	{
		convert_vectormap_to_table( vResult, l );
		return 2;
	}
}

/// reverse scan keys and values
// @function rscan
// @param instance ssdb::client
// @param key_start empty string or starter value
// @param key_end empty string or end value
// @param limit key limit size
// @return success true is success
// @return kv table with keys and values
// @see error_status
int ssdb_client_rscan( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 3 &&
		lua_type( l, 2 ) == LUA_TSTRING &&
		lua_type( l, 3 ) == LUA_TSTRING &&
		lua_type( l, 4 ) == LUA_TNUMBER );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sNameStart = lua_tostring( l, 2 );
	std::string sNameEnd = lua_tostring( l, 3 );
	uint64_t iLimit = lua_tonumber( l, 4 );
	std::vector<std::string> vResult;
	if ( error_status( l, pClient->rscan( sNameStart, sNameEnd, iLimit, &vResult ) ) )
		return 3;
	else
	{
		convert_vectormap_to_table( vResult, l );
		return 2;
	}
}

/// multi delete keys
// @function multi_del
// @param instance ssdb::client
// @param keys array table with keys
// @return success true is success
// @return keycount number of keys processed
// @see error_status
int ssdb_client_multi_del( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 2 ) == LUA_TTABLE );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::vector<std::string> aKeys;

	lua_pushvalue( l, -1 );
	lua_pushnil( l );
	while ( lua_next( l, -2 ) )
	{
		if ( lua_type( l, -1 ) == LUA_TSTRING )
			aKeys.push_back( lua_tostring( l, -1 ) );
		lua_pop( l, 1 );
	}
	lua_pop( l, 1 );

	if ( error_status( l, pClient->multi_del( aKeys ) ) )
		return 3;
	else
	{
		lua_pushnumber( l, aKeys.size() );
		return 2;
	}
}

/// multi set key/value
// @function multi_set
// @param instance ssdb::client
// @param keyval table with keys and values
// @return success true is success
// @return keycount number of keys processed
// @see error_status
int ssdb_client_multi_set( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 2 ) == LUA_TTABLE );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::map<std::string,std::string> aKeyVal;

	lua_pushvalue( l, -1 );
	lua_pushnil( l );
	while ( lua_next( l, -2 ) )
	{
		if ( lua_type( l, -2 ) == LUA_TNIL ) // only check key value not nil, other converted
			aKeyVal[ lua_tostring( l, -2 ) ] = lua_tostring( l, -1 );
		lua_pop( l, 1 );
	}
	lua_pop( l, 1 );

	if ( error_status( l, pClient->multi_set( aKeyVal ) ) )
		return 3;
	else
	{
		lua_pushnumber( l, aKeyVal.size() );
		return 2;
	}
}

/// multi get key/value
// @function multi_get
// @param instance ssdb::client
// @param keys table with keys
// @return success true is success
// @return kv_table table with keys and values
// @see error_status
int ssdb_client_multi_get( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 2 ) == LUA_TTABLE );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::vector<std::string> aKeys, aResult;
	convert_lua_table_vector( aKeys, l, -1 );
	if ( error_status( l, pClient->multi_get( aKeys, &aResult ) ) )
		return 3;
	else
	{
		convert_vectormap_to_table( aResult, l );
		return 2;
	}
}

/// get hash map key value
// @function hget
// @param instance ssdb::client
// @param hashmap_name
// @param key
// @return success true is success
// @return value
// @see error_status
int ssdb_client_hget( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	std::string sKey = lua_tostring( l, 3 );
	std::string sValue;
	if ( error_status( l, pClient->hget( sHashMap, sKey, &sValue ) ) )
		return 3;
	else
	{
		lua_pushstring( l, sValue.c_str() );
		return 2;
	}
}

/// set hash map key value
// @function hset
// @param instance ssdb::client
// @param hashmap_name
// @param key
// @param value
// @return success true is success
// @see error_status
int ssdb_client_hset( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 3 && 
		lua_type( l, 2 ) == LUA_TSTRING && 
		lua_type( l, 3 ) == LUA_TSTRING &&
	    lua_type( l, 4 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	std::string sKey = lua_tostring( l, 3 );
	std::string sValue = lua_tostring( l, 4 );

	if ( error_status( l, pClient->hset( sHashMap, sKey, sValue ) ) )
		return 3;
	else
	{
		return 1;
	}
}

/// delete hash map key
// @function hdel
// @param instance ssdb::client
// @param hashmap_name
// @param key
// @return success true is success
// @see error_status
int ssdb_client_hdel( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	std::string sKey = lua_tostring( l, 3 );
	if ( error_status( l, pClient->hdel( sHashMap, sKey ) ) )
		return 3;
	else
	{
		return 1;
	}
}

/// increment hash map key value by value
// @function hincr
// @param instance ssdb::client
// @param hashmap_name
// @param key
// @param inc increment value
// @return success true is success
// @return new_value incremented new value
// @see error_status
int ssdb_client_hincr( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 3 && 
		lua_type( l, 2 ) == LUA_TSTRING && 
		lua_type( l, 3 ) == LUA_TSTRING &&
		lua_type( l, 4 ) == LUA_TNUMBER );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	std::string sKey = lua_tostring( l, 3 );
	int64_t i64value = (uint64_t) lua_tonumber( l, 4 );
	int64_t i64NewValue = (uint64_t) lua_tonumber( l, 4 );
	if ( error_status( l, pClient->hincr( sHashMap, sKey,i64value, &i64NewValue ) ) )
		return 3;
	else
	{
		lua_pushnumber( l, i64NewValue );
		return 2;
	}
}

/// get size of hashmap
// @function hsize
// @param instance ssdb::client
// @param hashmap_name
// @return success true is success
// @return size size of hash map items
// @see error_status
int ssdb_client_hsize( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 2 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	int64_t iSize = 0;
	if ( error_status( l, pClient->hsize( sHashMap, &iSize ) ) )
		return 3;
	else
	{
		lua_pushnumber( l, iSize );
		return 2;
	}
}

/// clear hashmap
// @function hclear
// @param instance ssdb::client
// @param hashmap_name
// @return success true is success
// @return size size of cleared items
// @see error_status
int ssdb_client_hclear( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 2 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	int64_t iSize = 0;
	if ( error_status( l, pClient->hclear( sHashMap, &iSize ) ) )
		return 3;
	else
	{
		lua_pushnumber( l, iSize );
		return 2;
	}
}

/// get hashmap keys from start end range
// @function hkeys
// @param instance ssdb::client
// @param hashmap_name
// @param start_range
// @param end_range
// @param limit
// @return success true is success
// @return table with keys
// @see error_status
int ssdb_client_hkeys( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 4 &&
		lua_type( l, 2 ) == LUA_TSTRING &&
		lua_type( l, 3 ) == LUA_TSTRING &&
		lua_type( l, 4 ) == LUA_TSTRING &&
		lua_type( l, 5 ) == LUA_TNUMBER
	);

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	std::string sStartKey = lua_tostring( l, 3 );
	std::string sEndKey = lua_tostring( l, 4 );
	uint64_t iLimit = lua_tonumber( l, 5 );
	std::vector<std::string> aResults;

	if ( error_status( l, pClient->hkeys( sHashMap, sStartKey, sEndKey, iLimit, &aResults ) ) )
		return 3;
	else
	{
		convert_vectormap_to_table( aResults, l );
		return 2;
	}
}

/// get all hashmap key/value
// @function hgetall
// @param instance ssdb::client
// @param hashmap_name
// @return success true is success
// @return table with keys/values
// @see error_status
int ssdb_client_hgetall( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 2 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	std::vector<std::string> aResults;

	if ( error_status( l, pClient->hgetall( sHashMap, &aResults ) ) )
		return 3;
	else
	{
		convert_vectormap_to_table( aResults, l );
		return 2;
	}
}

/// scan all hasmap value
// @function hscan
// @param instance ssdb::client
// @param hashmap_name
// @param start_range
// @param end_range
// @param limit
// @return success true is success
// @return table with keys
// @see error_status
int ssdb_client_hscan( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 4 &&
		lua_type( l, 2 ) == LUA_TSTRING &&
		lua_type( l, 3 ) == LUA_TSTRING &&
		lua_type( l, 4 ) == LUA_TSTRING &&
		lua_type( l, 5 ) == LUA_TNUMBER
	);

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	std::string sStartKey = lua_tostring( l, 3 );
	std::string sEndKey = lua_tostring( l, 4 );
	uint64_t iLimit = lua_tonumber( l, 5 );
	std::vector<std::string> aResults;

	if ( error_status( l, pClient->hscan( sHashMap, sStartKey, sEndKey, iLimit, &aResults ) ) )
		return 3;
	else
	{
		convert_vectormap_to_table( aResults, l );
		return 2;
	}
}

/// reverse scan all hasmap value
// @function hkeys
// @param instance ssdb::client
// @param hashmap_name
// @param start_range
// @param end_range
// @param limit
// @return success true is success
// @return table with keys
// @see error_status
int ssdb_client_hrscan( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 4 &&
		lua_type( l, 2 ) == LUA_TSTRING &&
		lua_type( l, 3 ) == LUA_TSTRING &&
		lua_type( l, 4 ) == LUA_TSTRING &&
		lua_type( l, 5 ) == LUA_TNUMBER
	);

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	std::string sStartKey = lua_tostring( l, 3 );
	std::string sEndKey = lua_tostring( l, 4 );
	uint64_t iLimit = lua_tonumber( l, 5 );
	std::vector<std::string> aResults;

	if ( error_status( l, pClient->hrscan( sHashMap, sStartKey, sEndKey, iLimit, &aResults ) ) )
		return 3;
	else
	{
		convert_vectormap_to_table( aResults, l );
		return 2;
	}
}

/// hash map get multiple key/value
// @function multi_hget
// @param instance ssdb::client
// @param hashmap_name
// @param keys
// @return success true is success
// @return table with keys/values
// @see error_status
int ssdb_client_multi_hget( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TTABLE );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	std::vector<std::string> aKeys;
	std::vector<std::string> aResults;
	convert_lua_table_vector( aKeys, l, -1 );

	if ( error_status( l, pClient->multi_hget( sHashMap, aKeys, &aResults ) ) )
		return 3;
	else
	{
		convert_vectormap_to_table( aResults, l );
		return 2;
	}
}

/// hash map set multiple key/value
// @function multi_hset
// @param instance ssdb::client
// @param hashmap_name
// @param aKV
// @return success true is success
// @see error_status
int ssdb_client_multi_hset( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TTABLE );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	std::map<std::string, std::string> aKeysValues;
	convert_lua_table_map( aKeysValues, l, -1 );

	if ( error_status( l, pClient->multi_hset( sHashMap, aKeysValues ) ) )
		return 3;
	else
		return 1;
}

/// hash map del multiple key/value
// @function multi_del
// @param instance ssdb::client
// @param hashmap_name
// @param keys
// @return success true is success
// @see error_status
int ssdb_client_multi_hdel( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TTABLE );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sHashMap = lua_tostring( l, 2 );
	std::vector<std::string> aKeys;
	convert_lua_table_vector( aKeys, l, -1 );

	if ( error_status( l, pClient->multi_hdel( sHashMap, aKeys ) ) )
		return 3;
	else
		return 1;
}

/// pop item or items from list
// @function qpop
// @param instance ssdb::client
// @param list_name
// @param pop_size if size not defined, only one item get
// @return success true is success
// @return values if size not defined returns single value else returns table
// @see error_status
int ssdb_client_qpush( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 2 ) == LUA_TSTRING );
	
	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sListMap = lua_tostring( l, 2 );
	bool bMultipleValue = false;
	int64_t iSize = 1;

	if ( lua_gettop( l ) > 2 && lua_type( l, 3 ) == LUA_TNUMBER )
	{
		bMultipleValue = true;
	}

	std::string sValue;
	std::vector<std::string> aValues;
	
	ssdb::Status status;

	if ( bMultipleValue )
		status = pClient->qpop( sListMap, &sValue );
	else
		status = pClient->qpop( sListMap, iSize, &aValues );

	if ( error_status( l, status ) )
		return 3;
	else
	{
		if ( bMultipleValue )
			convert_vector_to_table( aValues, l );
		else
			lua_pushstring( l, sValue.c_str() );
		return 2;
	}
}

/// clear list item
// @function qclear
// @param instance ssdb::client
// @param list_name
// @return success true is success
// @see error_status
int ssdb_client_qclear( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 2 ) == LUA_TSTRING );
	
	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sListMap = lua_tostring( l, 2 );
	if ( error_status( l, pClient->qclear( sListMap, NULL ) ) )
		return 3;
	else
		return 1;
}

/// pop list
// @function qpop
// @param instance ssdb::client
// @param list_name
// @param limit
// @return success true is success
// @return value(s) returns table or value
// @see error_status
int ssdb_client_qpop( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 && lua_type( l, 2 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sListMap = lua_tostring( l, 2 );
	std::string Value;
	std::vector<std::string> aValues;
	ssdb::Status status;

	bool bMultiValues = lua_gettop( l ) > 2 && lua_type( l, 2 ) == LUA_TNUMBER;
	int64_t iLimit = 1;
	if ( bMultiValues )
		iLimit = lua_tointeger( l, 3 );

	if ( bMultiValues )
		status = pClient->qpop( sListMap, iLimit, &aValues );
	else
		status = pClient->qpop( sListMap, &Value );
	
	if ( error_status( l, status ) )
		return 3;
	else
	{
		if ( bMultiValues )
			convert_vector_to_table( aValues, l );
		else
			lua_pushstring( l, Value.c_str() );
		return 2;
	}
}

/// slice list
// @function qslice
// @param instance ssdb::client
// @param list_name
// @param posBegin
// @param posEnd
// @return success true is success
// @return values table with values
// @see error_status

int ssdb_client_qslice( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 3 && lua_type( l, 2 ) == LUA_TSTRING &&
		            lua_type( l, 3 ) == LUA_TNUMBER && lua_type( l, 4 ) == LUA_TNUMBER );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sListMap = lua_tostring( l, 2 );
	int64_t iStart = lua_tointeger( l, 3 );
	int64_t iEnd = lua_tointeger( l, 4 );

	std::vector<std::string> aValues;

	if ( error_status( l, pClient->qslice( sListMap, iStart, iEnd, &aValues ) ) )
		return 3;
	else
	{
		convert_vector_to_table( aValues, l );
		return 2;
	}
}

/// set the score in ordered set
// @function zset
// @param instance ssdb::client
// @param key
// @param value
// @param score
// @return success true is success
// @see error_status
int ssdb_client_zset( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 3 &&
				 lua_type( l, 2 ) == LUA_TSTRING &&
				 lua_type( l, 3 ) == LUA_TSTRING &&
				 lua_type( l, 4 ) == LUA_TNUMBER );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	std::string sValue = lua_tostring( l, 3 );
	int64_t score = lua_tonumber( l, 4 );

	if ( error_status( l, pClient->zset( sKey, sValue, score ) ) )
		return 3;
	else
		return 1;
}

/// get the score from spefied key
// @function zget
// @param instance ssdb::client
// @param key
// @param value
// @return success true is success
// @return score
// @see error_status
int ssdb_client_zget( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 &&
				 lua_type( l, 2 ) == LUA_TSTRING &&
				 lua_type( l, 3 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	std::string sValue = lua_tostring( l, 3 );
	int64_t score = 0;

	if ( error_status( l, pClient->zget( sKey, sValue, &score ) ) )
		return 3;
	else
	{
		lua_pushnumber( l, score );
		return 2;
	}
}

/// delete specified key from ordered list
// @function zdel
// @param instance ssdb::client
// @param key
// @param value
// @return success true is success
// @see error_status
int ssdb_client_zdel( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 &&
				 lua_type( l, 2 ) == LUA_TSTRING &&
				 lua_type( l, 3 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	std::string sValue = lua_tostring( l, 3 );

	if ( error_status( l, pClient->zdel( sKey, sValue ) ) )
		return 3;
	else
		return 1;
}

/// increment specified key score
// @function zincr
// @param instance ssdb::client
// @param key
// @param value
// @param increment
// @return success true is success
// @return new_score
// @see error_status
int ssdb_client_zincr( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 3 &&
				 lua_type( l, 2 ) == LUA_TSTRING &&
				 lua_type( l, 3 ) == LUA_TSTRING &&
				 lua_type( l, 4 ) == LUA_TNUMBER );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	std::string sValue = lua_tostring( l, 3 );
	int64_t score = lua_tonumber(l,4);

	if ( error_status( l, pClient->zincr( sKey, sValue, score, &score ) ) )
		return 3;
	else
	{
		lua_pushnumber( l, score );
		return 2;
	}
}

/// get zset size
// @function zsize
// @param instance ssdb::client
// @param key
// @return success true is success
// @return size
// @see error_status
int ssdb_client_zsize( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 2 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	int64_t iSize = 0;

	if ( error_status( l, pClient->zsize( sKey, &iSize ) ) )
		return 3;
	else
	{
		lua_pushnumber( l, iSize );
		return 2;
	}
}

/// clear zorder list
// @function zclear
// @param instance ssdb::client
// @param key
// @return success true is success
// @return delsize
// @see error_status
int ssdb_client_zclear( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 1 && lua_type( l, 2 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	int64_t iSize;

	if ( error_status( l, pClient->zclear( sKey, &iSize ) ) )
		return 3;
	else
	{
		lua_pushnumber( l, iSize );
		return 2;
	}
}

/// get items from zlist range
// @function zrange
// @param instance ssdb::client
// @param key
// @param offset
// @param limit
// @return success true is success
// @return table items in range
// @see error_status
int ssdb_client_zrange( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 3 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TNUMBER && lua_type( l, 4 ) == LUA_TNUMBER );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	int64_t iOffset  = lua_tonumber( l, 3 );
	int64_t iLimit   = lua_tonumber( l, 4 );
	std::vector<std::string> aResult;

	if ( error_status( l, pClient->zrange( sKey, iOffset,iLimit, &aResult ) ) )
		return 3;
	else
	{
		convert_vector_to_table( aResult, l );
		return 2;
	}
}

/// get items from zlist range, in reversed order
// @function zrrange
// @param instance ssdb::client
// @param key
// @param offset
// @param limit
// @return success true is success
// @return table items in range
// @see error_status
int ssdb_client_zrrange( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 3 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TNUMBER && lua_type( l, 4 ) == LUA_TNUMBER );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	int64_t iOffset = lua_tonumber( l, 3 );
	int64_t iLimit = lua_tonumber( l, 4 );
	std::vector<std::string> aResult;

	if ( error_status( l, pClient->zrrange( sKey, iOffset, iLimit, &aResult ) ) )
		return 3;
	else
	{
		convert_vector_to_table( aResult, l );
		return 2;
	}
}

/// get keys from zlist, with specified params
// @function zkeys
// @param instance ssdb::client
// @param key
// @param key_start
// @param score_start
// @param score_end
// @param limit
// @return success true is success
// @return table items in range
// @see error_status
int ssdb_client_zkeys( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 5 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK  ( l, 1 );
	std::string sKey      = lua_tostring( l, 2 );
	std::string sKeyStart = lua_tostring( l, 3 );
	int64_t iStart        = lua_tonumber( l, 4 );
	int64_t iEnd          = lua_tonumber( l, 5 );
	int64_t iLimit        = lua_tonumber( l, 6 );

	std::vector<std::string> aResult;

	if ( error_status( l, pClient->zkeys( sKey, sKeyStart, &iStart, &iEnd, iLimit, &aResult ) ) )
		return 3;
	else
	{
		convert_vector_to_table( aResult, l );
		return 2;
	}
}

/// scans values in zlist
// @function zscan
// @param instance ssdb::client
// @param key
// @param key_start
// @param score_start
// @param score_end
// @param limit
// @return success true is success
// @return table items in range
// @see error_status
int ssdb_client_zscan( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 5 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	std::string sKeyStart = lua_tostring( l, 3 );
	int64_t iStart = lua_tonumber( l, 4 );
	int64_t iEnd = lua_tonumber( l, 5 );
	int64_t iLimit = lua_tonumber( l, 6 );

	std::vector<std::string> aResult;

	if ( error_status( l, pClient->zscan( sKey, sKeyStart, &iStart, &iEnd, iLimit, &aResult ) ) )
		return 3;
	else
	{
		convert_vectormap_to_table( aResult, l , true);
		return 2;
	}
}

/// reverse scan vailes in ordered list
// @function zrscan
// @param instance ssdb::client
// @param key
// @param key_start
// @param score_start
// @param score_end
// @param limit
// @return success true is success
// @return table items in range
// @see error_status
int ssdb_client_zrscan( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 5 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TSTRING );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	std::string sKeyStart = lua_tostring( l, 3 );
	int64_t iStart = lua_tonumber( l, 4 );
	int64_t iEnd = lua_tonumber( l, 5 );
	int64_t iLimit = lua_tonumber( l, 6 );

	std::vector<std::string> aResult;

	if ( error_status( l, pClient->zrscan( sKey, sKeyStart, &iStart, &iEnd, iLimit, &aResult ) ) )
		return 3;
	else
	{
		convert_vectormap_to_table( aResult, l, true );
		return 2;
	}
}

/// get ordered list multiple value
// @function multi_zget
// @param instance ssdb::client
// @param key
// @param keys table with keys
// @return success true is success
// @return table items with score
// @see error_status
int ssdb_client_multi_zget( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 && lua_type( l, 2 ) == LUA_TSTRING);

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	std::vector<std::string> aKeys;
	
	if ( lua_type( l, 3 ) == LUA_TTABLE )
		convert_lua_table_vector( aKeys, l, -1 );
	else
		aKeys.push_back( lua_tostring( l, -1 ) );

	std::vector<std::string> aResult;
	if ( error_status( l, pClient->multi_zget( sKey, aKeys, &aResult ) ) )
		return 3;
	else
	{
		convert_vectormap_to_table( aResult, l, true );
		return 2;
	}
}

/// set ordered list values
// @function multi_zget
// @param instance ssdb::client
// @param key
// @param keys_score table
// @return success true is success
// @return table items in range
// @see error_status
int ssdb_client_multi_zset( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TTABLE );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	std::map<std::string, int64_t> aKeys;

	convert_lua_table_map( aKeys, l, -1 );
	if ( error_status( l, pClient->multi_zset( sKey, aKeys ) ) )
		return 3;
	else
		return 1;
}

/// set ordered list values
// @function multi_zget
// @param instance ssdb::client
// @param key
// @param keys_score table
// @return success true is success
// @return table items in range
// @see error_status
int ssdb_client_multi_zdel( lua_State* l )
{
	LUA_ASSERTL( l, lua_gettop( l ) > 2 && lua_type( l, 2 ) == LUA_TSTRING && lua_type( l, 3 ) == LUA_TTABLE );

	ssdb::Client* pClient = SSDB_CHECK( l, 1 );
	std::string sKey = lua_tostring( l, 2 );
	std::vector<std::string> aKeys;

	convert_lua_table_vector( aKeys, l, -1 );
	if ( error_status( l, pClient->multi_zdel( sKey, aKeys ) ) )
		return 3;
	else
		return 1;
}

//--------------------------------------------------------
static const luaL_Reg ssdb_metatable[] = {
	{ "dbsize",       ssdb_client_dbsize },
	{ "get_kv_range", ssdb_client_get_kv_range },
	{ "set_kv_range", ssdb_client_set_kv_range },
	{ "set",          ssdb_client_set_kv },
	{ "set_ttl",      ssdb_client_set_kv_ttl },
	{ "get",          ssdb_client_get_kv },
	{ "del",          ssdb_client_del_kv },
	{ "inc",          ssdb_client_inc_kv },
	{ "keys",         ssdb_client_keys },
	{ "scan",         ssdb_client_scan },
	{ "rscan",        ssdb_client_rscan },

	{ "multi_del",    ssdb_client_multi_del },
	{ "multi_set",    ssdb_client_multi_set },
	{ "multi_get",    ssdb_client_multi_get },

	{ "hget",       ssdb_client_hget },
	{ "hset",       ssdb_client_hset },
	{ "hdel",       ssdb_client_hdel },
	{ "hincr",      ssdb_client_hincr },
	{ "hsize",      ssdb_client_hsize },
	{ "hclear",     ssdb_client_hclear },
	{ "hkeys",      ssdb_client_hkeys },
	{ "hgetall",    ssdb_client_hgetall },
	{ "hscan",      ssdb_client_hscan },
	{ "hrscan",     ssdb_client_hrscan },
	{ "multi_hget", ssdb_client_multi_hget },
	{ "multi_hset", ssdb_client_multi_hset },
	{ "multi_hdel", ssdb_client_multi_hdel },
	{ "qpush",      ssdb_client_qpush },
	{ "qpop",       ssdb_client_qpop },
	{ "qclear",     ssdb_client_qclear },
	{ "qslice",     ssdb_client_qslice },


	{ "zset",       ssdb_client_zset },
	{ "zget",       ssdb_client_zget },
	{ "zset",       ssdb_client_zset },
	{ "zinc",       ssdb_client_zincr },
	{ "zdel",       ssdb_client_zdel },
	{ "zsize",      ssdb_client_zsize },
	{ "zclear",     ssdb_client_zclear },
	{ "zrrange",    ssdb_client_zrrange },
	{ "zrange",     ssdb_client_zrange },
	{ "keys",       ssdb_client_zkeys },
	{ "zscan",      ssdb_client_zscan },
	{ "zrscan",     ssdb_client_zrscan },

	{ "multi_zget", ssdb_client_multi_zget },
	{ "multi_zdel", ssdb_client_multi_zdel },
	{ "multi_zset", ssdb_client_multi_zset },



	{ NULL, NULL }
};

//--------------------------------------------------------
static const luaL_Reg ssdb_client[] = {
	{ "connect", ssdb_connect },
	{ NULL, NULL }
};

//--------------------------------------------------------
int luaopen_ssdb( lua_State *l )
{
#if defined(_WIN32) && defined( DNEED_WSA_STARTUP )
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 2, 2 );

	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 )
		return 1;
#endif

	luaL_newmetatable( l, DLUASSDBMETA );
	lua_newtable( l );
	luaL_register( l, NULL, ssdb_metatable );
	lua_setfield( l, -2, "__index" );
	lua_pushcfunction( l, ssdb_client_gc );
	lua_setfield( l, -2, "__gc" );

	luaL_register( l, DLUASSDBNAME, ssdb_client );
	return 1;
}