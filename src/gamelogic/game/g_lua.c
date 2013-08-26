/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Jeff Kent <jeff@jkent.net>

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#ifndef Q3_VM

#include "g_local.h"
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"


lua_State *g_luaState;

static char g_luaLineBuf[ 256 ];
static int g_luaLineBufLen;


/*
======================
CL_LuaWriteString
======================

The `l' parameter is ignored; lua guarantees the string to have a
terminating null.
*/
void G_LuaWriteString( const char *s, size_t l )
{
	char *p;
	int len;

	while ( p = strchr(s, '\n') )
	{
		len = p - s + 1;
		if ( g_luaLineBufLen + len <= sizeof( g_luaLineBuf ) - 1 )
		{
			strncpy( g_luaLineBuf + g_luaLineBufLen, s, len );
			g_luaLineBuf[g_luaLineBufLen + len] = '\0';
			trap_Print( g_luaLineBuf );
		}
		g_luaLineBufLen = 0;
		s = p + 1;
	}

	len = strlen( s );
	if ( len && g_luaLineBufLen + len <= sizeof( g_luaLineBuf ) - 2 )
	{
		strcpy( g_luaLineBuf + g_luaLineBufLen, s );
		g_luaLineBufLen += len;
	}
}

/*
======================
CL_LuaWriteLine
======================
*/
void G_LuaWriteLine( void )
{
	g_luaLineBuf[g_luaLineBufLen] = '\n';
	g_luaLineBuf[g_luaLineBufLen + 1] = '\0';
	trap_Print( g_luaLineBuf );
	g_luaLineBufLen = 0;
}

/*
=================
G_LuaInit
=================
*/
void G_LuaInit( void )
{
	if ( g_luaState )
	{
		G_Printf( "G_LuaInit: already initialized\n" );
		return;
	}

	g_luaLineBufLen = 0;
	g_luaState = luaL_newstate();
	if ( !g_luaState )
	{
		G_Printf("G_LuaInit: failed\n");
		return;
	}

	luaL_openlibs( g_luaState );
}

/*
=================
G_LuaCleanup
=================
*/
void G_LuaCleanup( void )
{
	if ( g_luaState )
	{
		lua_close( g_luaState );
		g_luaState = NULL;
	}
}

/*
=================
Svcmd_Lua_f
=================
*/
void Svcmd_Lua_f( void )
{
	char cmd[ 8 ];

	trap_Argv( 1, cmd, sizeof( cmd ) );

	if ( !Q_stricmp( cmd, "eval" ) )
	{
		char code[ MAX_TOKEN_CHARS ];
		trap_Argv( -2, code, sizeof( code ) );

		if ( luaL_dostring(g_luaState, code) )
		{
			G_Printf("lua eval: %s\n",
			lua_tostring(g_luaState, -1));
			lua_pop(g_luaState, 1);
			return;
		}
	}
	else if ( !Q_stricmp( cmd, "reset" ) )
	{
	}
	else if ( !Q_stricmp( cmd, "run" ) )
	{
	}
	else
	{
		G_Printf( "usage: lua (eval <code>|reset|run <file>)\n" );
	}
}

#endif /* Q3_VM */

