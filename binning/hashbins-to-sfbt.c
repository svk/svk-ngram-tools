#include "ngread.h"
#include "hashbin.h"
#include "sfbt.h"

#include "../lib/freegetopt-0.11/getopt.h"

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

long long ipow(long long a, long long b) {
    if( !b ) return 1;
    if( b == 1 ) return a;
    long long b1 = b/2;
    long long b2 = b - b1;
    return ipow( a, b1 ) * ipow( a, b2 );
}

int main(int argc, char* argv[]) {
    int N = -1;
    int no_bins = -1;
    char *TFN = 0;
    int verbose = 0;
    int confirmed = 0;
    char *ODN = "hashbins-to-sfbt.output";
    int prefix_digits = 0;
    char *suffix = ".sfbt";

    while(1) {
        int c = getopt( argc, argv, "vcn:o:B:p:U:" );
        if( c < 0 ) break;
        switch( c ) {
            case 'c':
                confirmed = 1;
                break;
            case 'U':
                if( optarg ) {
                    suffix = optarg;
                } else {
                    suffix = "";
                }
                break;
            case 'o':
                ODN = optarg;
                break;
            case 'n':
                N = atoi( optarg );
                break;
            case 'B':
                no_bins = atoi( optarg );
                break;
            case 'p':
                prefix_digits = atoi( optarg );
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

    if( no_bins < 0 ) {
        fprintf( stderr, "fatal error: input argument -B (number of hash bins) missing\n" );
        return 1;
    }

    mkdir( ODN, 0744 );

    struct sfbt_wctx **histogram = 0;
    long long memreq = sizeof *histogram * ipow( no_bins, N );
    long long totalmemreq = memreq + sizeof **histogram * ipow( no_bins, N );
    long long maxindex = ipow( no_bins, N );
    int *xs = malloc(sizeof *xs * N );
    assert( xs );

    if( ipow( no_bins, N - prefix_digits ) > 10000 ) {
        fprintf( stderr, "fatal error: would create too many files per directory (use more prefix digits)\n" );
        return 1;
    } 

    if( totalmemreq > (10 * 1024 * 1024) && !confirmed ) {
        fprintf( stderr, "fatal error: need %lld bytes of memory, confirm with -c\n", totalmemreq );
        return 1;
    } else {
        fprintf( stderr, "Estimated memory usage: %d bytes\n", totalmemreq );
    }

    histogram = malloc( memreq );
    if( !histogram ) {
        fprintf( stderr, "fatal error: unable to allocate %lld bytes of memory for histogram\n", memreq );
        return 1;
    }

    if( verbose ) {
        fprintf( stderr, "Parameters:\n" );
        fprintf( stderr, "\tn: %d-grams\n", N );
        fprintf( stderr, "\tB: %d hash bins\n", no_bins );
        if( verbose ) {
            fprintf( stderr, "\tinput from:\n" );
            for(int j=argi;j<argc;j++) {
                fprintf( stderr, "\t\t\"%s\"\n", argv[j] );
            }
            fprintf( stderr, "\toutput to:\n" );
        }
    }

    for(long long j=0;j<maxindex;j++) {
        long long index = j;
        for(int k=0;k<N;k++) {
            int x = index % no_bins;
            index /= no_bins;
            xs[ N-1-k ] = x;
        }
        assert( N >= 1 );
        char filename[4096];
        strcpy( filename, ODN );
        if( prefix_digits > 0 ) {
            char buf[1024];
            strcpy( buf, "" );
            for(int k=0;k<prefix_digits;k++) {
                char buf2[8];
                sprintf( buf2, "%c", xs[k] + 'a' );
                strcat( buf, buf2 );
            }
            strcat( filename, "/" );
            strcat( filename, buf );
        }
        mkdir( filename, 0744 );
        strcat( filename, "/" );
        for(int k=prefix_digits;k<N;k++) {
            char buf2[8];
            sprintf( buf2, "%c", xs[k] + 'a' );
            strcat( filename, buf2 );
        }
        strcat( filename, suffix );
        if( verbose ) {
            fprintf( stderr, "\t\t\"%s\"\n", filename );
        }
        histogram[j] = sfbt_new_wctx( filename );
        if( !histogram[j] ) {
            fprintf( stderr, "fatal error: failed to construct #%d wctx (%s)\n", j, filename );
            return 1;
        }
    }

    long long processed = 0;

    for(int j=argi;j<argc;j++) {
        const char *IFN = argv[j];

        fprintf( stderr, "Reading %d-grams from \"%s\".\n", N, IFN );
        struct ngr_file *ngrf = ngr_open( IFN );
        while( ngr_next(ngrf) ) {
            if( ngr_columns(ngrf) != (N+1) ) {
                fprintf( stderr, "fatal error: expected %d columns, got %d\n", N + 1, ngr_columns(ngrf) );
                exit(1);
            }
            char buffer[ MAX_KEY_SIZE ];
            long long index = 0;
            strcpy( buffer, ngr_s_col( ngrf, 0 ) );
            for(int k=0;k<N;k++) {
                const char *s = ngr_s_col( ngrf, k );
                index *= no_bins;
                index += classify_uint32( no_bins, murmur_hash( s, strlen(s) ) );

                if( k > 0 ) {
                    strcat( buffer, " " );
                    strcat( buffer, s );
                }
            }
            sfbt_add_entry( histogram[ index ], buffer, ngr_ll_col( ngrf, N ) );
            ++processed;
        }
        ngr_free(ngrf);
    }

    if( verbose ) {
        fprintf( stderr, "%lld %d-grams processed.\n", processed, N );
    }
    
    for(long long j=0;j<maxindex;j++) {
        if( verbose ) {
            fprintf( stderr, "Finalizing SFBT %d of %d.\n", j + 1, maxindex );
        }
        int rv = sfbt_close_wctx( histogram[j] );
        if( rv ) {
            fprintf( stderr, "fatal error: failure.\n" );
            return 1;
        }
    }

    free(xs);
    free(histogram);
    
    fprintf( stderr, "Success.\n" );

    return 0;
}
