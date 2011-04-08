#include "bins.h"

#include <assert.h>
#include <string.h>

#include "../lib/freegetopt-0.11/getopt.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int B = 3;
    int K = 1000;
    char *FN = "bintable_test.out";
    int verbose = 0;

    while(1) {
        int c = getopt( argc, argv, "vk:o:b:" );
        if( c < 0 ) break;
        switch( c ) {
            case 'k':
                K = atoi( optarg );
                break;
            case 'b':
                B = atoi( optarg );
                break;
            case 'o':
                FN = optarg;
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "unknown option: '%c'\n", c );
                exit(1);
        }
    }

    long long TW = ((1+K)*K)/2;
    int argi = optind;
    char buf[1024];
    struct bin_cons *bc = make_bincons( TW, B );
    int i;

    if( verbose ) {
        fprintf( stderr, "Parameters:\n" );
        fprintf( stderr, "\tb: %d bins\n", B );
        fprintf( stderr, "\tk: counting 1 .. %d\n", K );
        fprintf( stderr, "\t   (total weight %lld)\n", TW );
        fprintf( stderr, "\to: output to \"%s\"\n", FN );
    }

    assert( bc );

    for(i=1;i<=K;i++) {
        memset( buf, 0, sizeof buf );
        snprintf( buf, sizeof buf, "%d", i );
        int err = update_bincons( bc, buf, i );
        assert( !err );
    }

    FILE *f = fopen( FN, "wb" );
    assert( f );
    bincons_write( bc, f );
    fclose( f );

    free_bincons( bc );

    f = fopen( FN, "rb" );
    assert( f );
    struct bin_table *bt = bintable_read( f );
    fclose( f );

    assert( bt );

    printf( "%d bins produced (expected %d).\n", bt->no_bins, B );
    printf( "%lld total weight (expected %lld).\n", bt->total_count, TW );
    for(i=0;i<bt->no_bins;i++) {
        printf( "\t%d:\t% 12lld\t%s\n", i, bt->counts[i], bt->entries[i] );
    }
    printf( "bintable written to \"%s\" for further inspection.\n", FN );

    free_bintable( bt );
    return 0;
}
