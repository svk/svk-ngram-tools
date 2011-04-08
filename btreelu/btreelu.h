#ifndef H_BTREELU
#define H_BTREELU

#define TOKEN_SIZE 240
#define MAX_RECORDS 128
#define FILENAME_SIZE 8
#define RECORD_SIZE 256

#include <stdint.h>

struct btree_record {
    char token[ TOKEN_SIZE ];
    uint64_t weight;
    char filename[ FILENAME_SIZE ];
};

struct btree_cached_record {
    int n;
    struct btree_record data[MAX_RECORDS];
    struct btree_cached_record *cache_next[MAX_RECORDS];
};

void btree_free_cache( struct btree_cached_record* );

struct btree_cached_record* btree_make_cache( const char*, const char *, int );

int64_t btree_lookup(struct btree_cached_record *, const char *, const char *);

#endif
