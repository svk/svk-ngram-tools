#include "lib1tquery.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if( argc < 2 ) {
        fprintf( stderr, "Usage: test-lib1tquery configfile.ini\n" );
        return 1;
    }

    const char *configfile = argv[1];

    char buffer[4096], keybuf[4096];

    int failure = lib1tquery_init( configfile );
    if( failure ) {
        fprintf( stderr, "Failed to initialize, aborting.\n" );
        return 1;
    }

    while(1) {
        if(!fgets( buffer, 4096, stdin )) break;
        char *key = strtok( buffer, "\t\n" );
        strcpy( keybuf, key );
        char *colkey = strtok( buffer, " \t" );
        int i = 0;
        int toknotfound = 0;
        int32_t iseq[5];

        while( colkey ) {
            iseq[i] = lib1tquery_dictionary( colkey );

            i++;
            colkey = strtok( 0, " \t" );
        }

        uint64_t rv = lib1tquery_lookup_ngram( i, iseq );

        fprintf( stderr, "%llu\n", rv );
    }

    lib1tquery_quit();
}
