#ifndef H_SORTREAD
#define H_SORTREAD

#define MAX_SYMBOLS 5

#include <stdint.h>

void judysort_initialize(void);
void judysort_insert( const uint32_t*, int, int64_t );

void judysort_insert_test(int,int,int,int,int, int64_t);

int judysort_dump_output_test(int *, int, int64_t, void*);
int judysort_dump_output_gzfile(int *, int, int64_t, void *);

long long judysort_dump_free(int, int (*f)(int*,int,int64_t,void*), void* );

int judysort_get_count(void);

#endif
