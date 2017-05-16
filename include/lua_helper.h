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
#pragma once
extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include <assert.h>

void Stack_dump( lua_State* L, int iMin )
{

	int top = lua_gettop( L );
	int i;

	fprintf( stderr, "\n\tSTACK DEBUG:\n" );

	if ( top == 0 )
		fprintf( stderr, "\t(none)\n" );

	for ( i = iMin == 0 ? 1 : iMin; i <= top; i++ )
	{
		int type = lua_type( L, i );

		fprintf( stderr, "\t[%d]= (%s) ", i, lua_typename( L, type ) );
		lua_getglobal( L, "tostring" );
		if ( !lua_isfunction( L, -1 ) )
		{
			fprintf( stderr, "('tostring' not available)" );
		}
		else
		{
			lua_pushvalue( L, i );
			lua_call( L, 1 /*args*/, 1 /*retvals*/ );
			fprintf( stderr, "%s", lua_tostring( L, -1 ) );
		}
		lua_pop( L, 1 );
		fprintf( stderr, "\n" );
	}
	fprintf( stderr, "\n" );
}

#ifdef _DEBUG
#define LUA_STACK_ASSERT(L,O,N,D) luaL_error(L,"Stack size error ( func : %s old : %d , new : %d diff : %d ) in file : %s line : %d \n", __FUNCTION__, O,N,D, __FILE__,__LINE__);
#define LUA_ASSERT( L,VAL,MSG, ...) if ( !( VAL ) ) { luaL_error(L,MSG,##__VA_ARGS__); }
#define LUA_ASSERTLM( L,VAL,MSG) if ( !( VAL ) ) { luaL_error(L,MSG##" ( file : %s line : %d )\n", __FILE__, __LINE__); }
#define LUA_ASSERTL( L,VAL) if ( !( VAL ) ) { luaL_error(L," Debug assert ( func : %s file : %s line : %d )\n", __FUNCTION__, __FILE__, __LINE__); }

#define LUA_STACK_BEGIN(L) int __stack_old = lua_gettop(L); int __stack_new = 0; int __stack_diff = 0;
#define LUA_STACK_CHECK(L,N) __stack_diff = N;\
	__stack_new = lua_gettop(L);\
	if ( __stack_new - __stack_diff != __stack_old )\
{ LUA_STACK_ASSERT(L,__stack_old, __stack_new, __stack_diff); return N; }

#define LUA_STACK_CHECKNO_RETURN(L,N) __stack_diff = N;\
	__stack_new = lua_gettop(L);\
	if ( __stack_new - __stack_diff != __stack_old )\
{ LUA_STACK_ASSERT(L,__stack_old, __stack_new, __stack_diff); }

#define LUA_STACK_RETURN(L,N) LUA_STACK_CHECK(L,N) else { return N; }
#define LUA_STACK_STATUS(L,S) __stack_new = lua_gettop(L); printf( "stack %s new : %d diff : %d \n", S, __stack_new, __stack_new - __stack_old );
#define LUA_STACK_END() assert( __stack_new != 0 );
#define LUA_STACK_DUMP(L) Stack_dump(L,0);
#define LUA_CLEANUP(L) __stack_new = lua_gettop(L); lua_pop(l,__stack_new - __stack_old );

#else
#define LUA_STACK_DUMP(L)
#define LUA_STACK_STATUS(L,S)
#define LUA_ASSERTL( L,VAL)
#define LUA_ASSERTLM( L,VAL,MSG)	
#define LUA_STACK_BEGIN(L)
#define LUA_STACK_CHECK(L,N)
#define LUA_STACK_RETURN(L,N) return N;
#define LUA_STACK_ASSERT(L,O,N,D)
#define LUA_ASSERT( L,VAL,MSG, ...)
#define LUA_STACK_END()
#define LUA_CLEANUP(L)
#define LUA_STACK_CHECKNO_RETURN(L,N)
#endif

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif