#include "bins.h"
#include "ngread.h"

#include "../lib/freegetopt-0.11/getopt.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
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


    int *selcat = malloc(sizeof *selcat * N);
    for(int k=0;k<N;k++) {
        if( (k+argi) >= argc ) {
            fprintf( stderr, "fatal error: not enough arguments provided (n-gram bin)\n" );
            return 1;
        }
        selcat[k] = atoi( argv[k+argi] );
    }

    argi += N;

    if( N < 0 ) {
        fprintf( stderr, "fatal error: input argument -n (length of n-grams) missing\n" );
        return 1;
    }

    if( !TFN ) {
        fprintf( stderr, "fatal error: input argument -T (bin table) missing\n" );
        return 1;
    }
    struct bin_table *bt = bintable_read_fn( TFN );


    if( verbose ) {
        fprintf( stderr, "Parameters:\n" );
        fprintf( stderr, "\tn: %d-grams\n", N );
        fprintf( stderr, "\tinput from:\n" );
        for(int j=argi;j<argc;j++) {
            fprintf( stderr, "\t\t\"%s\"\n", argv[j] );
        }
    }

    long long processed = 0;

    for(int j=argi;j<argc;j++) {
        const char *IFN = argv[j];

        struct ngr_file *ngrf = ngr_open( IFN );
        while( ngr_next(ngrf) ) {
            if( ngr_columns(ngrf) != (N+1) ) {
                fprintf( stderr, "fatal error: expected %d columns, got %d\n", N + 1, ngr_columns(ngrf) );
                exit(1);
            }
            long long index = 0;
            int nodisplay = 0;
            for(int k=0;k<N;k++) {
                if( selcat[k] != bt_classify( bt, ngr_s_col( ngrf, k ) ) ) {
                    nodisplay = 1;
                    break;
                }
            }
            if( !nodisplay ) {
                for(int k=0;k<N;k++) {
                    if( k ) fputs( " ", stdout );
                    fputs( ngr_s_col( ngrf, k ), stdout );
                }
                fputs( "\n", stdout );
            }
            ++processed;
        }
        ngr_free(ngrf);
    }

    if( verbose ) {
        fprintf( stderr, "%lld %d-grams processed.\n", processed, N );
    }

    free_bintable( bt );
    free(selcat);

    return 0;
}
