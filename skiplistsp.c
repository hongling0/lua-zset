#include "skiplistsp.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>  // For int64_t definition

#define SKIPLIST_MAXLEVEL 32
#define SKIPLIST_P 0.25

int sp_compareScores(struct skiplist_sp *sl, int64_t score1[2], int64_t score2[2]) {
    for (int i = 0; i < 2; i++) {
        if (score1[i] < score2[i]) {
            return sl->cmp[i] ? 1 : -1;
        }
        if (score1[i] > score2[i]) {
            return sl->cmp[i] ? -1 : 1;
        }
    }
    return 0;
}

struct skiplistNode_sp *sp_slCreateNode(int level, int64_t score[2], int64_t obj) {
    struct skiplistNode_sp *n = malloc(sizeof(*n) + level * sizeof(struct skiplistLevel));
    n->score[0] = score[0];
    n->score[1] = score[1];
    n->obj = obj;
    return n;
}

void sp_slFreeNode(struct skiplistNode_sp *node) {
    free(node);
}

struct skiplist_sp *sp_slCreate(char cmp0, char cmp1) {
    int j;
    struct skiplist_sp *sl;

    sl = malloc(sizeof(*sl));
    sl->level = 1;
    sl->length = 0;
    sl->cmp[0] = cmp0;
    sl->cmp[1] = cmp1;
    int64_t zeroScore[2] = {0, 0};
    sl->header = sp_slCreateNode(SKIPLIST_MAXLEVEL, zeroScore, 0);
    for (j = 0; j < SKIPLIST_MAXLEVEL; j++) {
        sl->header->level[j].forward = NULL;
        sl->header->level[j].span = 0;
    }
    sl->header->backward = NULL;
    sl->tail = NULL;
    return sl;
}

void sp_slFree(struct skiplist_sp *sl) {
    struct skiplistNode_sp *node = sl->header->level[0].forward, *next;
    free(sl->header);
    while (node) {
        next = node->level[0].forward;
        sp_slFreeNode(node);
        node = next;
    }
    free(sl);
}

static int sp_slRandomLevel(void) {
    int level = 1;
    while ((random() & 0xffff) < (SKIPLIST_P * 0xffff))
        level += 1;
    return (level < SKIPLIST_MAXLEVEL) ? level : SKIPLIST_MAXLEVEL;
}

void sp_slInsert(struct skiplist_sp *sl, int64_t score[2], int64_t obj) {
    struct skiplistNode_sp *update[SKIPLIST_MAXLEVEL], *x;
    unsigned int rank[SKIPLIST_MAXLEVEL];
    int i, level;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        rank[i] = i == (sl->level - 1) ? 0 : rank[i + 1];
        while (x->level[i].forward && 
               (sp_compareScores(sl, x->level[i].forward->score, score) < 0 || 
                (sp_compareScores(sl, x->level[i].forward->score, score) == 0 && 
                 (x->level[i].forward->obj - obj) < 0))) {
            rank[i] += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    level = sp_slRandomLevel();
    if (level > sl->level) {
        for (i = sl->level; i < level; i++) {
            rank[i] = 0;
            update[i] = sl->header;
            update[i]->level[i].span = sl->length;
        }
        sl->level = level;
    }
    x = sp_slCreateNode(level, score, obj);
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
}

void sp_slDeleteNode(struct skiplist_sp *sl, struct skiplistNode_sp *x, struct skiplistNode_sp **update) {
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

int sp_slDelete(struct skiplist_sp *sl, int64_t score[2], int64_t obj) {
    struct skiplistNode_sp *update[SKIPLIST_MAXLEVEL], *x;
    int i;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
    while (x->level[i].forward && 
           (sp_compareScores(sl, x->level[i].forward->score, score) < 0 || 
           (sp_compareScores(sl, x->level[i].forward->score, score) == 0 && 
           (x->level[i].forward->obj - obj) < 0))) {
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    x = x->level[0].forward;
    if (x && sp_compareScores(sl, score, x->score) == 0 && (x->obj == obj)) {
        sp_slDeleteNode(sl, x, update);
        sp_slFreeNode(x);
        return 1;
    }
    return 0;
}

unsigned long sp_slDeleteByRank(struct skiplist_sp *sl, unsigned int start, unsigned int end, slDeleteCb cb, void *ud) {
    struct skiplistNode_sp *update[SKIPLIST_MAXLEVEL], *x;
    unsigned long traversed = 0, removed = 0;
    int i;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        while (x->level[i].forward && (traversed + x->level[i].span) < start) {
            traversed += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }

    traversed++;
    x = x->level[0].forward;
    while (x && traversed <= end) {
        struct skiplistNode_sp *next = x->level[0].forward;
        sp_slDeleteNode(sl, x, update);
        cb(ud, x->obj);
        sp_slFreeNode(x);
        removed++;
        traversed++;
        x = next;
    }
    return removed;
}

unsigned long sp_slGetRank(struct skiplist_sp *sl, int64_t score[2], int64_t o) {
    struct skiplistNode_sp *x;
    unsigned long rank = 0;
    int i;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        while (x->level[i].forward && 
               (sp_compareScores(sl, x->level[i].forward->score, score) < 0 || 
                (sp_compareScores(sl, x->level[i].forward->score, score) == 0 && 
                 (x->level[i].forward->obj - o) <= 0))) {
            rank += x->level[i].span;
            x = x->level[i].forward;
        }

        if (x->obj && (x->obj == o)) {
            return rank;
        }
    }
    return 0;
}

struct skiplistNode_sp *sp_slGetNodeByRank(struct skiplist_sp *sl, unsigned long rank) {
    if (rank == 0 || rank > sl->length) {
        return NULL;
    }

    struct skiplistNode_sp *x;
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

int sp_slIsInRange(struct skiplist_sp *sl, int64_t min[2], int64_t max[2]) {
    struct skiplistNode_sp *x;

    if (sp_compareScores(sl, min, max) > 0) {
        return 0;
    }
    x = sl->tail;
    if (x == NULL || sp_compareScores(sl, x->score, min) < 0)
        return 0;

    x = sl->header->level[0].forward;
    if (x == NULL || sp_compareScores(sl, x->score, max) > 0)
        return 0;
    return 1;
}

struct skiplistNode_sp *sp_slFirstInRange(struct skiplist_sp *sl, int64_t min[2], int64_t max[2]) {
    struct skiplistNode_sp *x;
    int i;

    if (!sp_slIsInRange(sl, min, max))
        return NULL;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        while (x->level[i].forward && sp_compareScores(sl, x->level[i].forward->score, min) < 0)
            x = x->level[i].forward;
    }

    x = x->level[0].forward;
    return x;
}

struct skiplistNode_sp *sp_slLastInRange(struct skiplist_sp *sl, int64_t min[2], int64_t max[2]) {
    struct skiplistNode_sp *x;
    int i;

    if (!sp_slIsInRange(sl, min, max))
        return NULL;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        while (x->level[i].forward && sp_compareScores(sl, x->level[i].forward->score, max) <= 0)
            x = x->level[i].forward;
    }

    return x;
}

unsigned long sp_slGetRankByScore(struct skiplist_sp *sl, int64_t score[2]) {
    if (sl->length == 0) {
        return 0;
    }

    struct skiplistNode_sp *x;
    unsigned long rank = 0;
    int i;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        while (x->level[i].forward && 
               sp_compareScores(sl, x->level[i].forward->score, score) < 0) {
            rank += x->level[i].span;
            x = x->level[i].forward;
        }
    }
    return rank + 1;
}
