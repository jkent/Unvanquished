#define lentitylib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lentitylib.h"

#include "../g_local.h"

#define VECTOROBJ "vec_t*"

#define tovectorobj(L, n)	((vec_t *)luaL_checkudata(L, n, VECTOROBJ))
#define getvectordim(L, n)  (lua_rawlen(L, n) / sizeof(vec_t))

static vec_t *newvector (lua_State *L, int dim) {
  vec_t *v;

  v = (vec_t *)lua_newuserdata(L, sizeof(vec_t) * dim);
  memset(v, 0, sizeof(vec_t) * dim);
  luaL_setmetatable(L, VECTOROBJ);
  return v;
}


static int vector_distance (lua_State *L) {
  vec_t *v1 = tovectorobj(L, 1);
  vec_t *v2 = tovectorobj(L, 2);
  int dim1 = getvectordim(L, 1);
  int dim2 = getvectordim(L, 2);

  if (dim1 != 3)
	return luaL_argerror(L, 1, VECTOROBJ " must have 3 dimensions");

  if (dim2 != 3)
    return luaL_argerror(L, 2, VECTOROBJ " must have 3 dimensions");

  lua_pushnumber(L, (lua_Number)Distance(v1, v2));
  return 1;
}


static int vector_call (lua_State *L) {
  int nargs = lua_gettop(L) - 1;
  int i, isnum;
  vec_t *v;

  luaL_checkany(L, 2);

  if (nargs == 1) {
    switch (lua_type(L, 2)) {
    case LUA_TTABLE:
      lua_len(L, 2);
      nargs = lua_tointeger(L, -1);
      lua_pop(L, 1);
      v = newvector(L, nargs);
      for (i = 0, isnum = 1; isnum && i < nargs; i++) {
    	lua_pushinteger(L, i + 1);
    	lua_gettable(L, 2);
        v[i] = (vec_t)lua_tonumberx(L, -1, &isnum);
        lua_pop(L, 1);
      }
      if (!isnum) {
        const char *msg = lua_pushfstring(L, "expected %s at table index %d",
                                          lua_typename(L, LUA_TNUMBER), i + 1);
        return luaL_argerror(L, 2, msg);

      }
      return 1;

    case LUA_TUSERDATA:
      return 0;
      break;
    }
  }

  v = newvector(L, nargs);
  for (i = 0, isnum = 1; isnum && i < nargs; i++)
    v[i] = (vec_t)luaL_checknumber(L, i + 2);
  return 1;
}


static int vectorobj_index (lua_State *L) {
  vec_t *v = tovectorobj(L, 1);
  int dim, i;
  const char *key;

  dim = lua_rawlen(L, 1) / sizeof(vec_t);

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
  int dim = lua_rawlen(L, 1) / sizeof(vec_t);
  lua_pushinteger(L, dim);
  return 1;
}


static int vectorobj_newindex (lua_State *L) {
  vec_t *v = tovectorobj(L, 1);
  int dim, i;
  const char *key;

  dim = lua_rawlen(L, 1) / sizeof(vec_t);

  switch (lua_type(L, 2)) {
  case LUA_TSTRING:
    key = lua_tostring(L, 2);
    if (dim >= 3 && strcmp("z", key) == 0)
      v[2] = (vec_t)lua_tonumber(L, 2);
    else if (dim >= 2 && strcmp("y", key) == 0)
      v[1] = (vec_t)lua_tonumber(L, 2);
    else if (dim >= 1 && strcmp("x", key) == 0)
      v[0] = (vec_t)lua_tonumber(L, 2);
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
  int i, dim = lua_rawlen(L, 1) / sizeof(vec_t);
  luaL_Buffer b;

  luaL_buffinit(L, &b);
  lua_pushfstring(L, VECTOROBJ "[%d] (", dim);
  luaL_addvalue(&b);
  for (i = 0; i < dim; i++) {
	if (i < dim - 1)
	  lua_pushfstring(L, "%f, ", (lua_Number) v[i]);
	else
      lua_pushfstring(L, "%f)", (lua_Number) v[i]);
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
