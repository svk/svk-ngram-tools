#ifndef H_LIB1TQUERY
#define H_LIB1TQUERY

#include <stdint.h>

void lib1tquery_quit(void);
int lib1tquery_init(const char*);

int32_t lib1tquery_dictionary( const char* );
int64_t lib1tquery_lookup_ngram( int, const int32_t* );

int64_t lib1tquery_lookup_2gram( int32_t, int32_t );
int64_t lib1tquery_lookup_3gram( int32_t, int32_t, int32_t );
int64_t lib1tquery_lookup_4gram( int32_t, int32_t, int32_t, int32_t );
int64_t lib1tquery_lookup_5gram( int32_t, int32_t, int32_t, int32_t, int32_t );

#endif
