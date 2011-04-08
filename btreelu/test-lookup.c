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

    int64_t rv = btree_lookup( "mytree", lustring );

    printf( "%s\t%lld\n", lustring, rv );

    return 0;
}
