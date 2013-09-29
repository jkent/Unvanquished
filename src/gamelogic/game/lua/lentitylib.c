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
#include "lvectorlib.h"

#include "../g_local.h"


#define ENTITYKEY "_ENTITIES"
#define ENTITYOBJ "gentity_t*"

typedef struct {
  gentity_t *entity;
  void (*pain)(gentity_t *self, gentity_t *attacker, int damage);
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

static void pain_hook(gentity_t *self, gentity_t *attacker, int damage) {
  lua_State *L = g_luaState;
  EntityObj *p;
  qboolean handled = qfalse;

  luaL_getsubtable(L, LUA_REGISTRYINDEX, ENTITYKEY);
  lua_rawgetp(L, -1, self);
  p = (EntityObj *)lua_touserdata(L, -1);
  if (!p) {
    G_Printf("Lost binding.\n");
    lua_settop(g_luaState, 0);
    return;
  }

  lua_pop(L, 1);
  lua_rawgetp(L, -1, p->pain);
  if (!lua_isfunction(L, -1))
    goto done;

  lua_pushcfunction(L, G_LuaTraceback);
  lua_insert(L, 1);
  pushentity(L, self);
  pushentity(L, attacker);
  lua_pushinteger(L, damage);
  if (lua_pcall(L, 3, 1, 1)) {
    G_Printf("lua pain hook: %s\n", lua_tostring(g_luaState, -1));
    goto done;
  }

  handled = lua_toboolean(L, -1);

done:
  if (p->pain && !handled)
    p->pain(self, attacker, damage);

  lua_settop(g_luaState, 0);
}

static gentity_t *G_FindClosestEntityOfClass( vec3_t origin, const char *classname )
{
	gentity_t *entity = NULL;
	gentity_t *closestEntity = NULL;
	float d, nd;

	while ( ( entity = G_IterateEntities( entity, classname, qfalse, 0, NULL ) ) )
	{
		nd = DistanceSquared( origin, entity->s.origin );

		if ( !closestEntity || nd < d )
		{
			d = nd;
			closestEntity = entity;
		}
	}
	return closestEntity;
}


static gentity_t *G_IterateEntitiesOfNameAndClass( gentity_t *entity, const char *name, const char *classname )
{
	while ( ( entity = G_IterateEntities( entity, classname, qfalse, 0, NULL ) ) )
	{
		if ( G_MatchesName( entity, name ) )
			return entity;
	}
	return NULL;
}


void G_SetAngles( gentity_t *self, const vec3_t angles )
{
	VectorCopy(angles, self->s.apos.trBase);
	self->s.apos.trType = TR_STATIONARY;
	self->s.apos.trTime = 0;
	self->s.apos.trDuration = 0;
	VectorClear(self->s.apos.trDelta);

	VectorCopy(angles, self->r.currentAngles);
	VectorCopy(angles, self->s.angles);
}


static int entity_iterator (lua_State *L) {
  EntityObj *p = (EntityObj *)lua_touserdata(L, lua_upvalueindex(1));
  const char *classname = lua_tostring(L, lua_upvalueindex(2));
  gentity_t *entity = NULL;

  if (p)
    entity = p->entity;

  entity = G_IterateEntities(entity, classname, qfalse, 0, NULL);
  if (!entity)
    return 0;

  pushentity(L, entity);
  lua_pushvalue(L, -1);
  lua_replace(L, lua_upvalueindex(1));
  return 1;
}


static int entity_iterate (lua_State *L) {
  lua_pushnil(L);
  lua_pushvalue(L, 1);
  lua_pushcclosure(L, entity_iterator, 2);
  return 1;
}


static int entity_find (lua_State *L) {
  gentity_t *entity = NULL;
  const char *name, *classname;
  vec_t *v;
  int i;

  luaL_checkany(L, 1);
  classname = lua_tostring(L, 2);

  switch (lua_type(L, 1)) {
  case LUA_TNIL:
    entity = G_IterateEntitiesOfClass(NULL, classname);
    break;

  case LUA_TNUMBER:
    i = lua_tonumber(L, 1);
    if ( i >= 0 && i < level.num_entities )
      entity = &g_entities[i];
    break;

  case LUA_TSTRING:
    name = lua_tostring(L, 1);
    entity = G_IterateEntitiesOfNameAndClass(NULL, name, classname);
    break;

  case LUA_TUSERDATA:
	v = lua_tovectorobj(L, 1);
	if (lua_getvectordim(L, 1) != 3)
      luaL_argerror(L, 1, LUA_VECTOROBJ " with 3 dimensions required");
	entity = G_FindClosestEntityOfClass(v, classname);
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
  lua_pushfstring(L, ENTITYOBJ " %s", etos(p->entity));
  return 1;
}


static int entityobj_activate (lua_State *L) {
  EntityObj *p = toentityobj(L);
  gentity_t *world = &g_entities[ENTITYNUM_WORLD];
  if (p->entity->act) {
    p->entity->nextAct = 0;
    p->entity->active = qtrue;
    p->entity->act(p->entity, world, world);
    p->entity->active = qfalse;
  }
  return 0;
}


static void entityobj_angles_getter(lua_State *L, EntityObj *p) {
  vec_t *v = lua_newvector(L, 3);
  memcpy(v, p->entity->s.angles, sizeof(vec_t) * 3);
}


static void entityobj_angles_setter(lua_State *L, EntityObj *p) {
  vec_t *v = lua_tovectorobj(L, 3);
  if (lua_getvectordim(L, 3) == 3) {
    G_SetAngles(p->entity, v);
    trap_LinkEntity(p->entity);
  }
  else
    luaL_argerror(L, 3, LUA_VECTOROBJ " with 3 dimensions required");
}


static void entityobj_bbox_getter(lua_State *L, EntityObj *p) {
  vec_t *v;

  lua_newtable(L);
  lua_pushinteger(L, 1);
  v = lua_newvector(L, 3);
  memcpy(v, p->entity->r.mins, sizeof(vec_t) * 3);
  lua_settable(L, -3);

  lua_pushinteger(L, 2);
  v = lua_newvector(L, 3);
  memcpy(v, p->entity->r.maxs, sizeof(vec_t) * 3);
  lua_settable(L, -3);
}


static void entityobj_origin_getter(lua_State *L, EntityObj *p) {
  vec_t *v = lua_newvector(L, 3);
  memcpy(v, p->entity->s.origin, sizeof(vec_t) * 3);
}


static void entityobj_origin_setter(lua_State *L, EntityObj *p) {
  vec_t *v = lua_tovectorobj(L, 3);
  if (lua_getvectordim(L, 3) == 3) {
    G_SetOrigin(p->entity, v);
    trap_LinkEntity(p->entity);
  }
  else
    luaL_argerror(L, 3, LUA_VECTOROBJ " with 3 dimensions required");
}


static void entityobj_pain_getter(lua_State *L, EntityObj *p) {
  luaL_getsubtable(L, LUA_REGISTRYINDEX, ENTITYKEY);
  lua_rawgetp(L, -1, p->pain);
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    lua_pushnil(L);
  }
  lua_remove(L, -2);
}


static void entityobj_pain_setter(lua_State *L, EntityObj *p) {
  if (!lua_isfunction(L, 3)) {
    if (p->entity->pain == pain_hook) {
      p->entity->pain = p->pain;
      p->pain = NULL;
    }
    return;
  }

  if (p->entity->pain != pain_hook) {
    p->pain = p->entity->pain;
    p->entity->pain = pain_hook;
  }

  luaL_getsubtable(L, LUA_REGISTRYINDEX, ENTITYKEY);
  lua_insert(L, 3);
  lua_rawsetp(L, -2, p->pain);
}


typedef struct {
  char *key;
  void (*getter)(lua_State *L, EntityObj *p);
  void (*setter)(lua_State *L, EntityObj *p);
} entityobj_var_t;

static entityobj_var_t entityobj_var_list[] = {
  {"angles",  entityobj_angles_getter,  entityobj_angles_setter },
  {"bbox",    entityobj_bbox_getter,    NULL                    },
  {"origin",  entityobj_origin_getter,  entityobj_origin_setter },
  {"pain",    entityobj_pain_getter,    entityobj_pain_setter   },
  {NULL,      NULL,                     NULL                    }
};


static int entityobj_index (lua_State *L) {
  EntityObj *p = toentityobj(L);
  const char *key = lua_tostring(L, 2);
  entityobj_var_t *var;

  if (key) {
    /* try to fetch from metatable */
    if (luaL_getmetafield(L, 1, key))
      return 1;

    /* else, find a getter */
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
  EntityObj *p = toentityobj(L);
  const char *key = lua_tostring(L, 2);
  entityobj_var_t *var;

  /* find a setter */
  if (key) {
    for (var = entityobj_var_list; var->key; var++) {
      if (strcmp(var->key, key) == 0) {
        if (var->setter)
          var->setter(L, p);
      }
    }
  }

  return 0;
}


/*
** functions for 'entity' library
*/
static const luaL_Reg entitylib[] = {
  {"find", entity_find},
  {"iterate", entity_iterate},
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
