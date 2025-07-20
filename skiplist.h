#ifndef SKIPLIST_HH
#define SKIPLIST_HH

typedef struct skiplistNode {
    int64_t obj;
    double score;
    struct skiplistNode *backward;
    struct skiplistLevel {
        struct skiplistNode *forward;
        unsigned int span;
    } level[];
} skiplistNode;

typedef struct skiplist {
    struct skiplistNode *header, *tail;
    unsigned long length;
    int level;
    char cmp;
} skiplist;

typedef void (*slDeleteCb)(void *ud, int64_t obj);
void slFreeNode(skiplistNode *node);

skiplist *slCreate(void);
void slFree(skiplist *sl);

void slInsert(skiplist *sl, double score, int64_t obj);
int slDelete(skiplist *sl, double score, int64_t obj);
unsigned long slDeleteByRank(skiplist *sl, unsigned int start, unsigned int end, slDeleteCb cb, void *ud);

unsigned long slGetRank(skiplist *sl, double score, int64_t o);
skiplistNode *slGetNodeByRank(skiplist *sl, unsigned long rank);

skiplistNode *slFirstInRange(skiplist *sl, double min, double max);
skiplistNode *slLastInRange(skiplist *sl, double min, double max);

int slCompareScores(skiplist *sl, double score1, double score2);
unsigned long slGetRankByScore(skiplist *sl, double score);

#endif //SKIPLIST_HH
