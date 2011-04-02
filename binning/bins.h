#ifndef H_BINS
#define H_BINS

#include <stdio.h>

struct bin_consentry {
    char *first_token;
    long long count;
    struct bin_consentry *next_bin;
};

struct bin_cons {
    long long total_count;
    long long bin_threshold;
    long long remaining_count;
    int bin_count;
    int target_bin_count;
    struct bin_consentry *first_bin;
    struct bin_consentry *last_bin;
};

struct bin_table {
    int no_bins;
    long long total_count;

    char **entries;
    long long *counts;
};

struct bin_cons* make_bincons(long long, int);
int update_bincons(struct bin_cons*, const char*, long long);
void free_bincons(struct bin_cons*);
int bincons_write(struct bin_cons *, FILE*);

struct bin_table* bintable_read(FILE*);
void free_bintable(struct bin_table*);

int bt_classify(struct bin_table*, const char*);

#endif
