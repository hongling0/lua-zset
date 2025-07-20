#ifndef SKIPLIST_SP_HH
#define SKIPLIST_SP_HH

#include <stdint.h>

struct skiplistNode_sp {
    int64_t obj;
    int64_t score[2];
    struct skiplistNode_sp *backward;
    struct skiplistLevel {
        struct skiplistNode_sp *forward;
        unsigned int span;
    } level[];
};

struct skiplist_sp {
    struct skiplistNode_sp *header, *tail;
    unsigned long length;
    int level;
    char cmp[2];
};

typedef void (*slDeleteCb)(void *ud, int64_t obj);
void sp_slFreeNode(struct skiplistNode_sp *node);
int sp_compareScores(struct skiplist_sp *sl, int64_t score1[2], int64_t score2[2]);

struct skiplist_sp *sp_slCreate(char cmp0, char cmp1);
void sp_slFree(struct skiplist_sp *sl);

void sp_slInsert(struct skiplist_sp *sl, int64_t score[2], int64_t obj);
int sp_slDelete(struct skiplist_sp *sl, int64_t score[2], int64_t obj);
unsigned long sp_slDeleteByRank(struct skiplist_sp *sl, unsigned int start, unsigned int end, slDeleteCb cb, void *ud);

unsigned long sp_slGetRank(struct skiplist_sp *sl, int64_t score[2], int64_t o);
struct skiplistNode_sp *sp_slGetNodeByRank(struct skiplist_sp *sl, unsigned long rank);

struct skiplistNode_sp *sp_slFirstInRange(struct skiplist_sp *sl, int64_t min[2], int64_t max[2]);
struct skiplistNode_sp *sp_slLastInRange(struct skiplist_sp *sl, int64_t min[2], int64_t max[2]);

unsigned long sp_slGetRankByScore(struct skiplist_sp *sl, int64_t score[2]);

#endif //SKIPLIST_SP_HH
