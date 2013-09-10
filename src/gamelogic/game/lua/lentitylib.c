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


#define ENTITYKEY "_ENTITIES"
#define ENTITYOBJ "ENTITY*"

typedef struct {
  gentity_t *entity;
  int test;
} EntityObj;

#define toentityobj(L)	((EntityObj *)luaL_checkudata(L, 1, ENTITYOBJ))


static EntityObj *pushentity (lua_State *L, gentity_t *entity) {
  EntityObj *p = NULL;

  /* lookup the entity object */
  luaL_getsubtable(L, LUA_REGISTRYINDEX, ENTITYKEY);
  lua_rawgetp(L, -1, entity);
  p = (EntityObj *)lua_touserdata(L, -1);
  if (p) {
    lua_remove(L, -2);
    return p;
  }

  /* not found, create one and store it */
  lua_pop(L, 1);
  p = (EntityObj *)lua_newuserdata(L, sizeof(EntityObj));
  p->entity = entity;
  p->test = -1;
  luaL_setmetatable(L, ENTITYOBJ);
  lua_pushvalue(L, -1);
  lua_rawsetp(L, -3, entity);
  lua_remove(L, -2);
  return p;
}


static gentity_t *G_FindClosestEntityOfClass( vec3_t origin, const char *classname )
{
	gentity_t *ent = NULL;
	gentity_t *closestEnt = NULL;
	float d, nd;

	while ( ( ent = G_IterateEntitiesOfClass( ent, classname ) ) )
	{
		nd = DistanceSquared( origin, ent->s.origin );

		if ( !closestEnt || nd < d )
		{
			d = nd;
			closestEnt = ent;
		}
	}
	return closestEnt;
}


static gentity_t *G_IterateEntitiesOfNameAndClass( gentity_t *entity, const char *name, const char *classname )
{
	while ( ( entity = G_IterateEntitiesOfClass( entity, classname ) ) )
	{
		if ( G_MatchesName( entity, name ) )
			return entity;
	}
	return NULL;
}


static int entity_find (lua_State *L) {
  gentity_t *entity = NULL;
  const char *name, *classname;
  vec3_t origin;
  int i;

  luaL_checkany(L, 1);

  switch (lua_type(L, 1)) {
  case LUA_TNIL:
    name = NULL;
    classname = lua_tostring(L, 2);
    entity = G_IterateEntitiesOfClass(NULL, classname);
    break;

  case LUA_TNUMBER:
    i = lua_tonumber(L, 1);
    if ( i >= 0 && i < level.num_entities )
      entity = &g_entities[i];
    break;

  case LUA_TSTRING:
    name = lua_tostring(L, 1);
    classname = lua_tostring(L, 2);
    entity = G_IterateEntitiesOfNameAndClass(NULL, name, classname);
    break;

  case LUA_TTABLE:
    for (i = 0; i < 3; i++) {
      lua_pushinteger(L, i + 1);
      lua_gettable(L, 1);
      origin[i] = (vec_t)lua_tonumber(L, -1);
      lua_pop(L, 1);
    }
    classname = lua_tostring(L, 2);
    entity = G_FindClosestEntityOfClass(origin, classname);
    break;
  }

  if (!entity)
    lua_pushnil(L);
  else
	pushentity(L, entity);

  return 1;
}


static int entityobj_tostring (lua_State *L) {
  EntityObj *p = toentityobj(L);
  lua_pushfstring(L, "entity %s", etos(p->entity));
  return 1;
}


static int entityobj_activate (lua_State *L) {
  EntityObj *p = toentityobj(L);
  G_FireEntity( p->entity, NULL );
  return 0;
}


static int entityobj_box (lua_State *L) {
	EntityObj *p = toentityobj(L);
	int n;

    lua_newtable(L);
    for (n = 0; n < 3; n++) {
      lua_pushnumber(L, p->entity->r.mins[n]);
      lua_rawseti(L, -2, n + 1);
    }

    lua_newtable(L);
    for (n = 0; n < 3; n++) {
      lua_pushnumber(L, p->entity->r.maxs[n]);
      lua_rawseti(L, -2, n + 1);
    }

    return 2;
}


static int entityobj_origin (lua_State *L) {
  EntityObj *p = toentityobj(L);
  int n;

  lua_newtable(L);
  for (n = 0; n < 3; n++) {
    lua_pushnumber(L, p->entity->s.origin[n]);
    lua_rawseti(L, -2, n + 1);
  }
  return 1;
}


/*
** functions for 'entity' library
*/
static const luaL_Reg entitylib[] = {
  {"find", entity_find},
  {NULL, NULL}
};


/*
** methods for entities
*/
static const luaL_Reg entityobj[] = {
  {"activate", entityobj_activate},
  {"box", entityobj_box},
  {"origin", entityobj_origin},
  {"__tostring", entityobj_tostring},
  {NULL, NULL}
};


static void createmeta (lua_State *L) {
  luaL_newmetatable(L, ENTITYOBJ);  /* create metatable for entity objects */
  lua_pushvalue(L, -1);  /* push metatable */
  lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
  luaL_setfuncs(L, entityobj, 0);  /* add file methods to new metatable */
  lua_pop(L, 1);  /* pop new metatable */

  luaL_newmetatable(L, ENTITYKEY);  /* create metatable for caching entity objects */
  lua_pushstring(L, "v");
  lua_setfield(L, -2, "__mode");  /* _ENTITIES.__mode = "k" */
  lua_pushvalue(L, -1);
  lua_setmetatable(L, -2);  /* table is its own metatable */
  lua_pop(L, 1);
}


LUAMOD_API int luaopen_entity (lua_State *L) {
	luaL_newlib(L, entitylib);
	createmeta(L);
	return 1;
}
