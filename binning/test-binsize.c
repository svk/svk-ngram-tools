#include "bins.h"
#include "ngread.h"

#include "freegetopt-0.11/getopt.h"

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

long long ipow(long long a, long long b) {
    if( !b ) return 1;
    if( b == 1 ) return a;
    long long b1 = b/2;
    long long b2 = b - b1;
    return ipow( a, b1 ) * ipow( a, b2 );
}

int main(int argc, char* argv[]) {
    int N = -1;
    char *TFN = 0;
    int verbose = 0;
    int confirmed = 0;
    while(1) {
        int c = getopt( argc, argv, "vn:T:" );
        if( c < 0 ) break;
        switch( c ) {
            case 'c':
                confirmed = 1;
                break;
            case 'n':
                N = atoi( optarg );
                break;
            case 'T':
                TFN = optarg;
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "unknown option: '%c'\n", c );
                exit(1);
        }
    }
    int argi = optind;

    if( N < 0 ) {
        fprintf( stderr, "fatal error: input argument -n (length of n-grams) missing\n" );
        return 1;
    }

    if( !TFN ) {
        fprintf( stderr, "fatal error: input argument -T (bin table) missing\n" );
        return 1;
    }
    struct bin_table *bt = bintable_read( TFN );

    long long *histogram = 0;
    long long memreq = sizeof *histogram * ipow( bt->no_bins, N );
    if( memreq > (10 * 1024 * 1024) && !confirmed ) {
        fprintf( stderr, "fatal error: need %lld bytes of memory for histogram, confirm with -c\n", memreq );
        return 1;
    }

    histogram = malloc( memreq );
    if( !histogram ) {
        fprintf( stderr, "fatal error: unable to allocate %lld bytes of memory for histogram\n", memreq );
        return 1;
    }
    memset( histogram, 0, memreq );

    if( verbose ) {
        fprintf( stderr, "Parameters:\n" );
        fprintf( stderr, "\tb: %d bins\n", B );
        fprintf( stderr, "\tn: %d-grams\n", N );
        fprintf( stderr, "\tinput from:\n" );
        for(int j=argi;j<argc;j++) {
            fprintf( stderr, "\t\t\"%s\"\n", argv[argi] );
        }
    }

    for(int j=argi;j<argc;j++) {
        const char *IFN = argv[argi];

        struct ngr_file *ngrf = ngr_open( IFN );
        while( ngr_next(ngrf) ) {
            assert( ngr_columns(ngrf) == (N+1) );
            long long index = 0;
            for(for k=0;k<N;k++) {
                index *= bt->no_bins;
            }
            int err = update_bincons( bc, ngr_s_col(ngrf,0), ngr_ll_col(ngrf,1) );
            assert( !err );
        }
        ngr_free(ngrf);
    }

    return 0;
}
