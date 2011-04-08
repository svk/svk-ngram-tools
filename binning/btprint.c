#include "bins.h"

#include "../lib/freegetopt-0.11/getopt.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
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

    while( argi < argc ) {
        const char *fn = argv[argi];
        FILE *f = fopen( fn, "rb" );
        if( !f ) {
            fprintf( stderr, "warning: failed to open \"%s\", skipping.\n", fn );
            continue;
        }
        struct bin_table *bt = bintable_read( f );
        if( !bt ) {
            fclose( f );
            fprintf( stderr, "warning: failed to read table in \"%s\", skipping.\n", fn );
            continue;
        }

        if( verbose ) {
            printf( "=== %s ===\n", fn );
        }

        printf( "%d bins, %lld total weight.\n", bt->no_bins, bt->total_count );
        for(int i=0;i<bt->no_bins;i++) {
            printf( "\t%d:\t% 12lld\t\"%s\"\n", i, bt->counts[i], bt->entries[i] );
        }

        free_bintable( bt );

        fclose( f );

        argi++;
    }

    return 0;
}
