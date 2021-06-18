#define LUA_LIB
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "lauxlib.h"
#include "lua.h"

#define SKIPLIST_MAXLEVEL 32
#define SKIPLIST_P 0.25

typedef struct skipsetNode {
    int64_t obj;
    struct skipsetNode *backward;
    struct skiplistLevel {
        struct skipsetNode *forward;
        unsigned int span;
    } level[];
} skipsetNode;

typedef struct skipset {
    struct skipsetNode *header, *tail;
    unsigned long length;
    int level;
} skipset;

static int slRandomLevel(void) {
    int level = 1;
    while ((random() & 0xffff) < (SKIPLIST_P * 0xffff))
        level += 1;
    return (level < SKIPLIST_MAXLEVEL) ? level : SKIPLIST_MAXLEVEL;
}

static skipsetNode *slCreateNode(int level, int64_t obj) {
    skipsetNode *n = malloc(sizeof(*n) + level * sizeof(struct skiplistLevel));
    n->obj = obj;
    return n;
}

static int slInsert(skipset *sl, int64_t obj) {
    skipsetNode *update[SKIPLIST_MAXLEVEL], *x;
    unsigned int rank[SKIPLIST_MAXLEVEL];
    int i, level;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        rank[i] = i == (sl->level - 1) ? 0 : rank[i + 1];
        while (x->level[i].forward && x->level[i].forward->obj < obj) {
            rank[i] += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    x = x->level[0].forward;
    if (x && x->obj == obj)
        return 0;

    level = slRandomLevel();
    if (level > sl->level) {
        for (i = sl->level; i < level; i++) {
            rank[i] = 0;
            update[i] = sl->header;
            update[i]->level[i].span = sl->length;
        }
        sl->level = level;
    }
    x = slCreateNode(level, obj);
    for (i = 0; i < level; i++) {
        x->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = x;

        x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
        update[i]->level[i].span = (rank[0] - rank[i]) + 1;
    }

    for (i = level; i < sl->level; i++) {
        update[i]->level[i].span++;
    }

    x->backward = (update[0] == sl->header) ? NULL : update[0];
    if (x->level[0].forward)
        x->level[0].forward->backward = x;
    else
        sl->tail = x;
    sl->length++;
    return 1;
}

static void slDeleteNode(skipset *sl, skipsetNode *x, skipsetNode **update) {
    int i;
    for (i = 0; i < sl->level; i++) {
        if (update[i]->level[i].forward == x) {
            update[i]->level[i].span += x->level[i].span - 1;
            update[i]->level[i].forward = x->level[i].forward;
        } else {
            update[i]->level[i].span -= 1;
        }
    }
    if (x->level[0].forward) {
        x->level[0].forward->backward = x->backward;
    } else {
        sl->tail = x->backward;
    }
    while (sl->level > 1 && sl->header->level[sl->level - 1].forward == NULL)
        sl->level--;
    sl->length--;
}

static void slFreeNode(skipsetNode *node) {
    free(node);
}

static int slDelete(skipset *sl, int64_t obj) {
    skipsetNode *update[SKIPLIST_MAXLEVEL], *x;
    int i;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        while (x->level[i].forward && x->level[i].forward->obj < obj)
            x = x->level[i].forward;
        update[i] = x;
    }

    x = x->level[0].forward;
    if (x && (x->obj == obj)) {
        slDeleteNode(sl, x, update);
        slFreeNode(x);
        return 1;
    } else {
        return 0; /* not found */
    }
    return 0; /* not found */
}

static unsigned long slGetRank(skipset *sl, int64_t obj) {
    skipsetNode *x;
    unsigned long rank = 0;
    int i;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        while (x->level[i].forward && x->level[i].forward->obj <= obj) {
            rank += x->level[i].span;
            x = x->level[i].forward;
        }
        if (x->obj && (x->obj == obj)) {
            return rank;
        }
    }
    return 0;
}

static skipsetNode *slGetNodeByRank(skipset *sl, unsigned long rank) {
    if (rank == 0 || rank > sl->length)
        return NULL;

    skipsetNode *x;
    unsigned long traversed = 0;
    int i;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        while (x->level[i].forward && (traversed + x->level[i].span) <= rank) {
            traversed += x->level[i].span;
            x = x->level[i].forward;
        }
        if (traversed == rank) {
            return x;
        }
    }

    return NULL;
}

unsigned long slDeleteByRank(skipset *sl, unsigned int rank) {
    skipsetNode *update[SKIPLIST_MAXLEVEL], *x;
    unsigned long traversed = 0;
    int i;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        while (x->level[i].forward && (traversed + x->level[i].span) < rank) {
            traversed += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }

    x = x->level[0].forward;
    if (x) {
        slDeleteNode(sl, x, update);
        slFreeNode(x);
        return 1;
    }
    return 0;
}

static skipset *slCreate(void) {
    int j;
    skipset *sl;

    sl = malloc(sizeof(*sl));
    sl->level = 1;
    sl->length = 0;
    sl->header = slCreateNode(SKIPLIST_MAXLEVEL, 0);
    for (j = 0; j < SKIPLIST_MAXLEVEL; j++) {
        sl->header->level[j].forward = NULL;
        sl->header->level[j].span = 0;
    }
    sl->header->backward = NULL;
    sl->tail = NULL;
    return sl;
}

static void slFree(skipset *sl) {
    skipsetNode *node = sl->header->level[0].forward, *next;
    free(sl->header);
    while (node) {
        next = node->level[0].forward;
        slFreeNode(node);
        node = next;
    }
    free(sl);
}

static inline skipset *
_to_skipset(lua_State *L, int idx) {
    skipset **sl = lua_touserdata(L, idx);
    if (sl == NULL) {
        luaL_error(L, "must be skipset object");
    }
    return *sl;
}

static int
_insert(lua_State *L) {
    skipset *sl = _to_skipset(L, 1);
    lua_Integer obj = luaL_checkinteger(L, 2);
    lua_pushboolean(L, slInsert(sl, obj));
    return 1;
}

static int
_delete(lua_State *L) {
    skipset *sl = _to_skipset(L, 1);
    lua_Integer obj = luaL_checkinteger(L, 2);
    lua_pushboolean(L, slDelete(sl, obj));
    return 1;
}

static int
_get_count(lua_State *L) {
    skipset *sl = _to_skipset(L, 1);
    lua_pushinteger(L, sl->length);
    return 1;
}

static int
_get_rank(lua_State *L) {
    skipset *sl = _to_skipset(L, 1);
    lua_Integer obj = luaL_checkinteger(L, 2);

    unsigned long rank = slGetRank(sl, obj);
    if (rank == 0) {
        return 0;
    }

    lua_pushinteger(L, rank);
    return 1;
}

static int
_get_by_rank(lua_State *L) {
    skipset *sl = _to_skipset(L, 1);
    unsigned long r1 = luaL_checkinteger(L, 2);

    skipsetNode *node = slGetNodeByRank(sl, r1);
    if (node) {
        lua_pushinteger(L, node->obj);
        return 1;
    }
    return 0;
}

static int
_delete_by_rank(lua_State *L) {
    skipset *sl = _to_skipset(L, 1);
    unsigned int rank = luaL_checkinteger(L, 2);
    lua_pushinteger(L, slDeleteByRank(sl, rank));
    return 1;
}

static int
_new(lua_State *L) {
    skipset *psl = slCreate();

    skipset **sl = (skipset **)lua_newuserdata(L, sizeof(skipset *));
    *sl = psl;
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_setmetatable(L, -2);
    return 1;
}

static int
_release(lua_State *L) {
    skipset *sl = _to_skipset(L, 1);
    slFree(sl);
    return 0;
}

LUAMOD_API int
luaopen_skipset_c(lua_State *L) {
    luaL_checkversion(L);

    luaL_Reg l[] = {
        { "insert", _insert },
        { "delete", _delete },
        { "delete_byrank", _delete_by_rank },

        { "count", _get_count },
        { "rank", _get_rank },
        { "byrank", _get_by_rank },

        { NULL, NULL }
    };

    lua_createtable(L, 0, 2);

    luaL_newlib(L, l);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, _release);
    lua_setfield(L, -2, "__gc");
    lua_pushcfunction(L, _get_count);
    lua_setfield(L, -2, "__len");

    lua_pushcclosure(L, _new, 1);
    return 1;
}
