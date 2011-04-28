#include "bins.h"
#include "ngread.h"

#include "../lib/freegetopt-0.11/getopt.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <assert.h>

#include "sha1.h"
#include <stdint.h>

int hash_classify( const int no_bins, const char *s ) {
    // Note that we probably don't need all the quality of SHA1,
    // a further optimization is swapping it out for a non-secure
    // but quicker hash with approximately as good a distribution.
    // done: this is FNV-1a
    const uint32_t p = 16777619;
    const uint32_t bs = (0xffffffffl / (long long unsigned) no_bins);
    const uint8_t *x = (const uint8_t*) s;
    uint32_t h = 2166136261;
    while( *x != '\0' ) {
        h ^= *x++;
        h *= p;
    }
    return (h / bs) % no_bins;
}

long long ipow(long long a, long long b) {
    if( !b ) return 1;
    if( b == 1 ) return a;
    long long b1 = b/2;
    long long b2 = b - b1;
    return ipow( a, b1 ) * ipow( a, b2 );
}

int main(int argc, char* argv[]) {
    int N = -1;
    int verbose = 0;
    int confirmed = 0;
    int no_bins = -1;
    while(1) {
        int c = getopt( argc, argv, "vn:B:" );
        if( c < 0 ) break;
        switch( c ) {
            case 'c':
                confirmed = 1;
                break;
            case 'n':
                N = atoi( optarg );
                break;
            case 'B':
                no_bins = atoi(optarg);
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

    if( no_bins < 0 ) {
        fprintf( stderr, "fatal error: input argument -B (number of bins) missing\n" );
        return 1;
    }
    long long *histogram = 0;
    long long memreq = sizeof *histogram * ipow( no_bins, N );
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
        fprintf( stderr, "\tn: %d-grams\n", N );
        fprintf( stderr, "\tinput from:\n" );
        for(int j=argi;j<argc;j++) {
            fprintf( stderr, "\t\t\"%s\"\n", argv[j] );
        }
        fprintf( stderr, "\t%lld bytes of memory required for histogram\n", memreq );
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
            for(int k=0;k<N;k++) {
                index *= no_bins;
                index += hash_classify( no_bins, ngr_s_col( ngrf, k ) );
            }
            histogram[ index ]++;
            ++processed;
        }
        ngr_free(ngrf);
    }

    if( verbose ) {
        fprintf( stderr, "%lld %d-grams processed.\n", processed, N );
    }

    int *xs = malloc(sizeof *xs * N );
    assert( xs );

    long long maxindex = ipow( no_bins, N );
    for(long long j=0;j<maxindex;j++) {
        long long index = j;
        long long val = histogram[index];
        printf( "%lld", val );
        for(int k=0;k<N;k++) {
            int x = index % no_bins;
            index /= no_bins;
            xs[ N-1-k ] = x;
        }
        for(int k=0;k<N;k++) {
            printf( "\t%d", xs[k] );
        }
        printf( "\n" );
    }

    free( xs );

    return 0;
}
