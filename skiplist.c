/*
 *  author: xjdrew
 *  date: 2014-06-03 20:38
 */

// skiplist similar with the version in redis
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "skiplist.h"

#define SKIPLIST_MAXLEVEL 32
#define SKIPLIST_P 0.25

int slCompareScores(skiplist *sl, double score1, double score2) {
    if (score1 < score2) {
        return sl->cmp ? 1 : -1;
    }
    if (score1 > score2) {
        return sl->cmp ? -1 : 1;
    }
    return 0;
}

skiplistNode *slCreateNode(int level, double score, int64_t obj) {
    skiplistNode *n = malloc(sizeof(*n) + level * sizeof(struct skiplistLevel));
    n->score = score;
    n->obj = obj;
    return n;
}

void slFreeNode(skiplistNode *node) {
    free(node);
}

skiplist *slCreate(void) {
    int j;
    skiplist *sl;

    sl = malloc(sizeof(*sl));
    sl->level = 1;
    sl->length = 0;
    sl->cmp = 0; // 默认升序
    sl->header = slCreateNode(SKIPLIST_MAXLEVEL, 0, 0);
    for (j = 0; j < SKIPLIST_MAXLEVEL; j++) {
        sl->header->level[j].forward = NULL;
        sl->header->level[j].span = 0;
    }
    sl->header->backward = NULL;
    sl->tail = NULL;
    return sl;
}

void slFree(skiplist *sl) {
    skiplistNode *node = sl->header->level[0].forward, *next;
    free(sl->header);
    while (node) {
        next = node->level[0].forward;
        slFreeNode(node);
        node = next;
    }
    free(sl);
}

static int slRandomLevel(void) {
    int level = 1;
    while ((random() & 0xffff) < (SKIPLIST_P * 0xffff))
        level += 1;
    return (level < SKIPLIST_MAXLEVEL) ? level : SKIPLIST_MAXLEVEL;
}

void slInsert(skiplist *sl, double score, int64_t obj) {
    skiplistNode *update[SKIPLIST_MAXLEVEL], *x;
    unsigned int rank[SKIPLIST_MAXLEVEL];
    int i, level;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        /* store rank that is crossed to reach the insert position */
        rank[i] = i == (sl->level - 1) ? 0 : rank[i + 1];
        while (x->level[i].forward && (slCompareScores(sl, x->level[i].forward->score, score) < 0 || (slCompareScores(sl, x->level[i].forward->score, score) == 0 && (x->level[i].forward->obj - obj) < 0))) {
            rank[i] += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    /* we assume the key is not already inside, since we allow duplicated
	 * scores, and the re-insertion of score and redis object should never
	 * happen since the caller of slInsert() should test in the hash table
	 * if the element is already inside or not. */
    level = slRandomLevel();
    if (level > sl->level) {
        for (i = sl->level; i < level; i++) {
            rank[i] = 0;
            update[i] = sl->header;
            update[i]->level[i].span = sl->length;
        }
        sl->level = level;
    }
    x = slCreateNode(level, score, obj);
    for (i = 0; i < level; i++) {
        x->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = x;

        /* update span covered by update[i] as x is inserted here */
        x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
        update[i]->level[i].span = (rank[0] - rank[i]) + 1;
    }

    /* increment span for untouched levels */
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

/* Internal function used by slDelete, slDeleteByScore */
void slDeleteNode(skiplist *sl, skiplistNode *x, skiplistNode **update) {
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

/* Delete an element with matching score/object from the skiplist. */
int slDelete(skiplist *sl, double score, int64_t obj) {
    skiplistNode *update[SKIPLIST_MAXLEVEL], *x;
    int i;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        while (x->level[i].forward && 
            (slCompareScores(sl, x->level[i].forward->score, score) < 0 || (slCompareScores(sl, x->level[i].forward->score, score) == 0 && (x->level[i].forward->obj - obj) < 0)))
            x = x->level[i].forward;
        update[i] = x;
    }
    /* We may have multiple elements with the same score, what we need
	 * is to find the element with both the right score and object. */
    x = x->level[0].forward;
    if (x && score == x->score && (x->obj == obj)) {
        slDeleteNode(sl, x, update);
        slFreeNode(x);
        return 1;
    }
    return 0; /* not found */
}

/* Delete all elements with rank between start and end (inclusive)
 * Note: ranks are 1-based */
unsigned long slDeleteByRank(skiplist *sl, unsigned int start, unsigned int end, slDeleteCb cb, void *ud) {
    skiplistNode *update[SKIPLIST_MAXLEVEL], *x;
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
        skiplistNode *next = x->level[0].forward;
        slDeleteNode(sl, x, update);
        cb(ud, x->obj);
        slFreeNode(x);
        removed++;
        traversed++;
        x = next;
    }
    return removed;
}

/* Get element rank by score and key
 * Returns: 0 if not found, 1-based rank otherwise 
 * (1-based due to header span) */
unsigned long slGetRank(skiplist *sl, double score, int64_t o) {
    skiplistNode *x;
    unsigned long rank = 0;
    int i;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        while (x->level[i].forward && (slCompareScores(sl, x->level[i].forward->score, score) < 0 || (slCompareScores(sl, x->level[i].forward->score, score) == 0 && (x->level[i].forward->obj - o) <= 0))) {
            rank += x->level[i].span;
            x = x->level[i].forward;
        }

        /* x might be equal to sl->header, so test if obj is non-NULL */
        if (x->obj && (x->obj == o)) {
            return rank;
        }
    }
    return 0;
}

/* Get element by its 1-based rank */
skiplistNode *slGetNodeByRank(skiplist *sl, unsigned long rank) {
    if (rank == 0 || rank > sl->length) {
        return NULL;
    }

    skiplistNode *x;
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

/* Check if any element is in score range [min, max] (inclusive) */
int slIsInRange(skiplist *sl, double min, double max) {
    skiplistNode *x;

    /* Test for ranges that will always be empty. */
    if (slCompareScores(sl, min, max) > 0) {
        return 0;
    }
    x = sl->tail;
    if (x == NULL || slCompareScores(sl, x->score, min) < 0)
        return 0;

    x = sl->header->level[0].forward;
    if (x == NULL || slCompareScores(sl, x->score, max) > 0)
        return 0;
    return 1;
}

/* Find the first node that is contained in the specified range.
 * Returns NULL when no element is contained in the range. */
skiplistNode *slFirstInRange(skiplist *sl, double min, double max) {
    skiplistNode *x;
    int i;

    /* If everything is out of range, return early. */
    if (!slIsInRange(sl, min, max))
        return NULL;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        /* Go forward while *OUT* of range. */
        while (x->level[i].forward && slCompareScores(sl, x->level[i].forward->score, min) < 0)
            x = x->level[i].forward;
    }

    /* This is an inner range, so the next node cannot be NULL. */
    x = x->level[0].forward;
    return x;
}

/* Find the last node that is contained in the specified range.
 * Returns NULL when no element is contained in the range. */
skiplistNode *slLastInRange(skiplist *sl, double min, double max) {
    skiplistNode *x;
    int i;

    /* If everything is out of range, return early. */
    if (!slIsInRange(sl, min, max))
        return NULL;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        /* Go forward while *IN* range. */
        while (x->level[i].forward && slCompareScores(sl, x->level[i].forward->score, max) <= 0)
            x = x->level[i].forward;
    }

    /* This is an inner range, so this node cannot be NULL. */
    return x;
}

unsigned long slGetRankByScore(skiplist *sl, double score) {
    skiplistNode *x;
    unsigned long rank = 0;
    int i;

    x = sl->header;
    for (i = sl->level - 1; i >= 0; i--) {
        while (x->level[i].forward && slCompareScores(sl, x->level[i].forward->score, score) <= 0) {
            rank += x->level[i].span;
            x = x->level[i].forward;
        }
    }
    return rank + 1;
}
