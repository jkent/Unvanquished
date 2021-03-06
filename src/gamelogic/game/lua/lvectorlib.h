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

#ifndef lvectorlib_h
#define lvectorlib_h

#include "lua.h"
#include "../g_local.h"


#define LUA_VECTORLIBNAME "vector"
#define LUA_VECTOROBJ "vec_t*"

#define lua_tovectorobj(L, n)	((vec_t *)luaL_checkudata(L, n, LUA_VECTOROBJ))
#define lua_getvectordim(L, n)	(lua_rawlen(L, n) / sizeof(vec_t))

LUAMOD_API int (luaopen_vector) (lua_State *L);
vec_t *lua_newvector (lua_State *L, int dim);


#endif /* lvectorlib_h */
