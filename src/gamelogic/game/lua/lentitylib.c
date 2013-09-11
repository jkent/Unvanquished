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


void entityobj_bbox_getter(lua_State *L, EntityObj *p) {
  int i;

  lua_newtable(L);
  lua_pushinteger(L, 1);
  lua_newtable(L);
  for (i = 0; i < 3; i++) {
    lua_pushinteger(L, i + 1);
    lua_pushnumber(L, p->entity->r.mins[i]);
    lua_settable(L, -3);
  }
  lua_settable(L, -3);

  lua_pushinteger(L, 2);
  lua_newtable(L);
  for (i = 0; i < 3; i++) {
    lua_pushinteger(L, i + 1);
    lua_pushnumber(L, p->entity->r.maxs[i]);
    lua_settable(L, -3);
  }
  lua_settable(L, -3);
}


void entityobj_origin_getter(lua_State *L, EntityObj *p) {
  int i;

  lua_newtable(L);
  for (i = 0; i < 3; i++) {
    lua_pushinteger(L, i + 1);
    lua_pushnumber(L, p->entity->s.origin[i]);
    lua_settable(L, -3);
  }
}


void entityobj_origin_setter(lua_State *L, EntityObj *p) {
  vec3_t origin;
  int i;

  for (i = 0; i < 3; i++) {
    lua_pushinteger(L, i + 1);
    lua_gettable(L, -2);
    origin[i] = (vec_t)lua_tonumber(L, -1);
    lua_pop(L, 1);
  }

  G_SetOrigin(p->entity, origin);
}


typedef struct {
  char *key;
  void (*getter)(lua_State *L, EntityObj *p);
  void (*setter)(lua_State *L, EntityObj *p);
} entityobj_var_t;

static entityobj_var_t entityobj_var_list[] = {
  {"bbox",   entityobj_bbox_getter,   NULL                   },
  {"origin", entityobj_origin_getter, entityobj_origin_setter},
  {NULL,     NULL,                    NULL                   }
};


static int entityobj_index (lua_State *L) {
  const char *key_list[] = {"bbox", "origin", NULL};
  void (*func_list[])(lua_State *L, EntityObj *p) = {entityobj_bbox_getter, entityobj_origin_getter, NULL};
  EntityObj *p = toentityobj(L);
  const char *key = lua_tostring(L, 2);
  entityobj_var_t *var;

  if (luaL_getmetafield(L, 1, key))
    return 1;

  if (key) {
    for (var = entityobj_var_list; var->key; var++) {
      if (strcmp(var->key, key) == 0) {
        if (!var->getter)
          break;
        var->getter(L, p);
        return 1;
      }
    }
  }

  lua_pushnil(L);
  return 1;
}


static int entityobj_newindex (lua_State *L) {
  const char *key_list[] = {"bbox", "origin", NULL};
  void (*func_list[])(lua_State *L, EntityObj *p) = {NULL, entityobj_origin_setter, NULL};
  EntityObj *p = toentityobj(L);
  const char *key = lua_tostring(L, 2);
  entityobj_var_t *var;

  /* look up key */
  if (key) {
    /* disallow setting metamethods */
    if (key[0] == '_' && key[1] == '_')
      return 0;

    for (var = entityobj_var_list; var->key; var++) {
      if (strcmp(var->key, key) == 0) {
        if (var->setter)
          var->setter(L, p);
        return 0;
      }
    }
  }

  /* else, set in table */
  lua_getmetatable(L, 1);
  lua_insert(L, 2);
  lua_rawset(L, -3);

  return 0;
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
  {"__index", entityobj_index},
  {"__newindex", entityobj_newindex},
  {"__tostring", entityobj_tostring},
  {NULL, NULL}
};


static void createmeta (lua_State *L) {
  luaL_newmetatable(L, ENTITYOBJ);  /* create metatable for entity objects */
  luaL_setfuncs(L, entityobj, 0);  /* add methods to metatable */
  lua_pop(L, 1);  /* pop metatable */

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
