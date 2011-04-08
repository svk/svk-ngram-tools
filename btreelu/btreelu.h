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

int64_t btree_lookup(const char *, const char *);

#endif
