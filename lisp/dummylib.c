// gcc -shared -o libdummy.so -fPIC dummylib.c

#include <stdint.h>
#include <stdio.h>

int dummylib_init(const char *s) {
    fprintf( stderr, "Dummylib initialized with parameter (config file?) \"%s\".\n", s );
    return 0;
}

void dummylib_quit(void) {
    fprintf( stderr, "Dummylib quit.\n" );
}

uint64_t dummylib_dictionary( const char *s ) {
    if( s[0] == '\0' ) return 0;
    return (int) s[0] + 256 * (int) s[1];
}

uint64_t dummylib_lookup_2gram( uint32_t a, uint32_t b ) {
    return a + b;
}

uint64_t dummylib_lookup_3gram( uint32_t a, uint32_t b, uint32_t c ) {
    return a + b + c;
}

uint64_t dummylib_lookup_4gram( uint32_t a, uint32_t b, uint32_t c, uint32_t d ) {
    return a + b + c + d;
}

uint64_t dummylib_lookup_5gram( uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e ) {
    return a + b + c + d + e;
}
