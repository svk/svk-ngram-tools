#include <stdio.h>

#include <assert.h>
#include <string.h>

#include "../lib/freegetopt-0.11/getopt.h"
#include "btreelu.h"

#include <stdlib.h>

int main(int argc, char *argv[]) {
    int verbose = 0;
    while(1) {
        int c = getopt( argc, argv, "v" );
        if( c < 0 ) break;
        switch( c ) {
            case 'v':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "unknown option: '%c'\n", c );
                exit(1);
        }
    }
    int argi = optind;

    char *lustring = "14 - day Total Weight";

    char buffer[ TOKEN_SIZE ];
    while(1) {
        char *buf = fgets( buffer, TOKEN_SIZE, stdin );
        if( !buf ) break;

        int buflen = strlen( buffer );
        if( buflen == 0 ) {
            break;
        } else if( buffer[buflen-1] == '\n' ) {
            buffer[buflen-1] = '\0';
            if( buflen == 1 ) break;
        } else {
            fprintf( stderr, "fatal error: input line too long\n" );
            return 1;
        }
        int64_t rv = btree_lookup( "mytree", buffer );
        printf( "%s\t%lld\n", buffer, rv );
    }

    return 0;
}