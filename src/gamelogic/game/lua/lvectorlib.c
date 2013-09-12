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

#define lentitylib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lentitylib.h"

#include "../g_local.h"


#define VECTOROBJ "vec_t*"

#define tovectorobj(L, n)	((vec_t *)luaL_checkudata(L, n, VECTOROBJ))
#define getvectordim(L, n)	(lua_rawlen(L, n) / sizeof(vec_t))


static vec_t *newvector (lua_State *L, int dim) {
  vec_t *v = (vec_t *)lua_newuserdata(L, sizeof(vec_t) * dim);
  memset(v, 0, sizeof(vec_t) * dim);
  luaL_setmetatable(L, VECTOROBJ);
  return v;
}


static int vector_distance (lua_State *L) {
  vec_t *v[2];
  int i, dim[2];

  for (i = 0; i < 2; i++) {
    v[i] = tovectorobj(L, i + 1);
    dim[i] = getvectordim(L, i + 1);
    if (dim[i] != 3)
      return luaL_argerror(L, i + 1, VECTOROBJ " with 3 dimensions required");
  }

  lua_pushnumber(L, (lua_Number)Distance(v[0], v[1]));
  return 1;
}


static int vector_call (lua_State *L) {
  int i, dim, isnum;
  vec_t *v, *vo;

  switch (lua_type(L, 2)) {
  case LUA_TTABLE:
	dim = lua_rawlen(L, 2);
    v = newvector(L, dim);
    for (i = 0, isnum = 1; isnum && i < dim; i++) {
      lua_pushinteger(L, i + 1);
      lua_rawget(L, 2);
      v[i] = (vec_t)lua_tonumberx(L, -1, &isnum);
    }
    if (!isnum) {
      const char *msg = lua_pushfstring(L, "%s expected at table index %d",
                                        lua_typename(L, LUA_TNUMBER), i);
      return luaL_argerror(L, 2, msg);
    }
    lua_pop(L, dim);
    break;

  case LUA_TUSERDATA:
    vo = tovectorobj(L, 2);
    dim = getvectordim(L, 2);
    v = newvector(L, dim);
    memcpy(v, vo, sizeof(vec_t) * dim);
    break;

  case LUA_TNUMBER:
    dim = lua_tointeger(L, 2);
    v = newvector(L, dim);
    memset(v, 0, sizeof(vec_t) * dim);
    break;

  default:
    lua_pushnil(L);
  }

  return 1;
}


static int vectorobj_index (lua_State *L) {
  vec_t *v = tovectorobj(L, 1);
  int i, dim = getvectordim(L, 1);
  const char *key;

  switch (lua_type(L, 2)) {
  case LUA_TSTRING:
    key = lua_tostring(L, 2);

    /* try to fetch from metatable */
    if (luaL_getmetafield(L, 1, key))
      break;

    /* else, try to use aliases */
    if (dim >= 3 && strcmp("z", key) == 0)
      lua_pushnumber(L, (lua_Number)v[2]);
    else if (dim >= 2 && strcmp("y", key) == 0)
      lua_pushnumber(L, (lua_Number)v[1]);
    else if (dim >= 1 && strcmp("x", key) == 0)
      lua_pushnumber(L, (lua_Number)v[0]);
    else
      lua_pushnil(L);
    break;

  case LUA_TNUMBER:
    i = lua_tointeger(L, 2) - 1;
    if (i >= 0 && i < dim)
      lua_pushnumber(L, (lua_Number)v[i]);
    else
      lua_pushnil(L);
    break;

  default:
    lua_pushnil(L);

  }
  return 1;
}


static int vectorobj_len (lua_State *L) {
  vec_t *v = tovectorobj(L, 1);
  int dim = getvectordim(L, 1);
  lua_pushinteger(L, dim);
  return 1;
}


static int vectorobj_newindex (lua_State *L) {
  vec_t *v = tovectorobj(L, 1);
  int i, dim = getvectordim(L, 1);
  const char *key;

  switch (lua_type(L, 2)) {
  case LUA_TSTRING:
    key = lua_tostring(L, 2);
    if (dim >= 3 && strcmp("z", key) == 0)
      v[2] = (vec_t)lua_tonumber(L, 3);
    else if (dim >= 2 && strcmp("y", key) == 0)
      v[1] = (vec_t)lua_tonumber(L, 3);
    else if (dim >= 1 && strcmp("x", key) == 0)
      v[0] = (vec_t)lua_tonumber(L, 3);
    break;

  case LUA_TNUMBER:
    i = lua_tointeger(L, 2) - 1;
    if (i >= 0 && i < dim && lua_isnumber(L, 3))
      v[i] = (vec_t)lua_tonumber(L, 3);
  }

  return 0;
}


static int vectorobj_tostring (lua_State *L) {
  vec_t *v = tovectorobj(L, 1);
  int i, dim = getvectordim(L, 1);
  luaL_Buffer b;

  luaL_buffinit(L, &b);
  lua_pushfstring(L, VECTOROBJ "[%d] {", dim);
  luaL_addvalue(&b);
  for (i = 0; i < dim; i++) {
    if (i < dim - 1)
      lua_pushfstring(L, "%f, ", (lua_Number) v[i]);
    else
      lua_pushfstring(L, "%f}", (lua_Number) v[i]);
    luaL_addvalue(&b);
  }
  luaL_pushresult(&b);
  return 1;
}


/*
** functions for 'vector' library
*/
static const luaL_Reg vectorlib[] = {
  {"distance", vector_distance},
  {"__call", vector_call},
  {NULL, NULL}
};


/*
** methods for vectors
*/
static const luaL_Reg vectorobj[] = {
  {"__index", vectorobj_index},
  {"__len", vectorobj_len},
  {"__newindex", vectorobj_newindex},
  {"__tostring", vectorobj_tostring},
  {NULL, NULL}
};


static void createmeta (lua_State *L) {
  lua_pushvalue(L, -1);
  lua_setmetatable(L, -2);  /* library as its own metatable */

  luaL_newmetatable(L, VECTOROBJ);  /* create metatable for vector objects */
  luaL_setfuncs(L, vectorobj, 0);  /* add methods to metatable */
  lua_pop(L, 1);  /* pop metatable */
}


LUAMOD_API int luaopen_vector (lua_State *L) {
  luaL_newlib(L, vectorlib);
  createmeta(L);
  return 1;
}
