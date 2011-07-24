#ifndef H_UNILOOKUP
#define H_UNILOOKUP

#include <pthread.h>
#include <stdint.h>

#include "sfbti.h"
#include "wordhash.h"

#define MIN_N
#define MAX_N 5

struct unilookup_tree_ctx {
    int present;

    int n;
    int prefix_digits;
    int bins;

    struct sfbti_rctx *files;
    int no_files;

    pthread_mutex_t mutex;
};

struct unilookup_ctx {
    WordHashCtx words;

    struct unilookup_tree_ctx tree[ MAX_N ];
};

#endif
