/*
 *  author: xjdrew
 *  date: 2014-06-03 20:38
 */

#include <stdio.h>
#include <stdlib.h>

#include "lauxlib.h"
#include "lua.h"
#include "skiplist.h"

static inline skiplist *
_to_skiplist(lua_State *L) {
    skiplist **sl = lua_touserdata(L, 1);
    if (sl == NULL) {
        luaL_error(L, "must be skiplist object");
    }
    return *sl;
}

static int
_insert(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    lua_Integer obj = luaL_checkinteger(L, 2);
    double score = luaL_checknumber(L, 3);
    slInsert(sl, score, obj);
    return 0;
}

static int
_delete(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    lua_Integer obj = luaL_checkinteger(L, 2);
    double score = luaL_checknumber(L, 3);
    lua_pushboolean(L, slDelete(sl, score, obj));
    return 1;
}

static void
_delete_rank_cb(void *ud, int64_t obj) {
    lua_State *L = (lua_State *)ud;
    lua_pushvalue(L, 4);
    lua_pushinteger(L, obj);
    lua_call(L, 1, 0);
}

static int
_delete_by_rank(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    unsigned int start = luaL_checkinteger(L, 2);
    unsigned int end = luaL_checkinteger(L, 3);
    luaL_checktype(L, 4, LUA_TFUNCTION);
    if (start > end) {
        unsigned int tmp = start;
        start = end;
        end = tmp;
    }

    lua_pushinteger(L, slDeleteByRank(sl, start, end, _delete_rank_cb, L));
    return 1;
}

static int
_get_count(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    lua_pushinteger(L, sl->length);
    return 1;
}

static int
_rank_byobj(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    lua_Integer obj = luaL_checkinteger(L, 2);
    double score = luaL_checknumber(L, 3);

    unsigned long rank = slGetRank(sl, score, obj);
    if (rank == 0) {
        return 0;
    }

    lua_pushinteger(L, rank);
    return 1;
}

static int
_ranks_byscore(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    double s1 = luaL_checknumber(L, 2);
    double s2 = luaL_checknumber(L, 3);

    skiplistNode *node = slFirstInRange(sl, s1, s2);
    if (node && slCompareScores(sl, node->score, s2) <= 0) {
        unsigned long start = slGetRank(sl, node->score, node->obj);
        lua_pushinteger(L, start);
        int n = 0;
        while (node && slCompareScores(sl, node->score, s2) <= 0) {
            n++;
            node = node->level[0].forward;
        }
        lua_pushinteger(L, start + n - 1);
        return 2;
    }
    return 0;
}

static int
_obj_byrank(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    unsigned long rank = luaL_checkinteger(L, 2);

    skiplistNode *node = slGetNodeByRank(sl, rank);
    if (node) {
        lua_pushinteger(L, node->obj);
        return 1;
    }
    return 0;
}

static int
_objs_byrank(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    unsigned long r1 = luaL_checkinteger(L, 2);
    unsigned long r2 = luaL_checkinteger(L, 3);
    
    if (r1 > r2) {
        luaL_error(L, "invalid rank range: r1(%lu) > r2(%lu)", r1, r2);
    }

    unsigned long rangelen = r2 - r1 + 1;
    skiplistNode *node = slGetNodeByRank(sl, r1);
    lua_createtable(L, rangelen, 0);
    int n = 0;
    while (node && n < rangelen) {
        n++;
        lua_pushinteger(L, node->obj);
        lua_rawseti(L, -2, n);
        node = node->level[0].forward;
    }
    return 1;
}


static int
_objs_byscore(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    double s1 = luaL_checknumber(L, 2);
    double s2 = luaL_checknumber(L, 3);

    skiplistNode *node = slFirstInRange(sl, s1, s2);
    lua_newtable(L);
    int n = 0;
    while (node && slCompareScores(sl, node->score, s2) <= 0) {
        n++;
        lua_pushinteger(L, node->obj);
        lua_rawseti(L, -2, n);
        node = node->level[0].forward;
    }
    return 1;
}

static int
_new(lua_State *L) {
    char cmp = luaL_optinteger(L, 1, 0);
    skiplist *psl = slCreate();
    psl->cmp = cmp;

    skiplist **sl = (skiplist **)lua_newuserdata(L, sizeof(skiplist *));
    *sl = psl;
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_setmetatable(L, -2);
    return 1;
}

static int
_release(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    //printf("collect sl:%p\n", sl);
    slFree(sl);
    return 0;
}

LUAMOD_API int
luaopen_skiplist_c(lua_State *L) {
    luaL_checkversion(L);

    luaL_Reg l[] = {
        { "insert", _insert },
        { "delete", _delete },
        { "delete_byrank", _delete_by_rank },

        { "get_count", _get_count },
        { "rank_byobj", _rank_byobj },
        { "ranks_byscore", _ranks_byscore },
        { "obj_byrank", _obj_byrank },
        { "objs_byrank", _objs_byrank },
        { "objs_byscore", _objs_byscore },

        { NULL, NULL }
    };

    lua_createtable(L, 0, 2);

    luaL_newlib(L, l);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, _release);
    lua_setfield(L, -2, "__gc");

    lua_pushcclosure(L, _new, 1);
    return 1;
}
