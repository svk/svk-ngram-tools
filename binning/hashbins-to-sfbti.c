#include "ngread.h"
#include "hashbin.h"
#include "sfbti.h"

#include "../lib/freegetopt-0.11/getopt.h"

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "wordhash.h"

#define DESCRIPTORS_PER_BATCH 1000
// limit tends to be 1024

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
    char *suffix = ".sfbti";
    char *vocfile = 0;

    while(1) {
        int c = getopt( argc, argv, "vcn:o:B:p:U:V:" );
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
            case 'V':
                vocfile = optarg;
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

    if( !vocfile ) {
        fprintf( stderr, "fatal error: input argument -V (vocabulary file) missing\n" );
        return 1;
    }

    mkdir( ODN, 0744 );

    struct sfbti_wctx **histogram = 0;
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
        fprintf( stderr, "\tV: %s vocabulary file\n", vocfile);
        if( verbose ) {
            fprintf( stderr, "\tinput from:\n" );
            for(int j=argi;j<argc;j++) {
                fprintf( stderr, "\t\t\"%s\"\n", argv[j] );
            }
            fprintf( stderr, "\toutput to:\n" );
        }
    }

    fprintf( stderr, "Generating token hashes...\n" );

    WordHashCtx words = read_wordhashes( vocfile );

    fprintf( stderr, "Finished generating token hashes.\n" );
    
    int passno = 1;
    long long fdno = 0;

    do {
        long long dmax = MIN( fdno + DESCRIPTORS_PER_BATCH, maxindex );
        fprintf( stderr, "Beginning pass %d, covering files %ld to %ld.\n", passno, fdno, dmax-1 );
        for(long long j=0;j<maxindex;j++) {
            histogram[j] = 0;
        }
        for(long long j=fdno;j<dmax;j++) {
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
            histogram[j] = sfbti_new_wctx( filename );
            if( !histogram[j] ) {
                fprintf( stderr, "fatal error: failed to construct #%d wctx (%s)\n", j, filename );
                return 1;
            }
        }

        long long processed = 0;
        long long hits = 0;

        for(int j=argi;j<argc;j++) {
            const char *IFN = argv[j];

            fprintf( stderr, "Reading %d-grams from \"%s\".\n", N, IFN );
            struct ngr_file *ngrf = ngr_open( IFN );
            while( ngr_next(ngrf) ) {
                if( ngr_columns(ngrf) != (N+1) ) {
                    fprintf( stderr, "fatal error: expected %d columns, got %d\n", N + 1, ngr_columns(ngrf) );
                    exit(1);
                }
                char buffer[ 2048 ];
                long long index = 0;
                strcpy( buffer, ngr_s_col( ngrf, 0 ) );
                int isequence[ N ];

                for(int k=0;k<N;k++) {
                    const char *s = ngr_s_col( ngrf, k );
                    index *= no_bins;
                    index += classify_uint32( no_bins, murmur_hash( s, strlen(s) ) );

                    isequence[k] = lookup_wordhash( words, s );

                    assert( isequence[k] >= 0 );

                    if( k > 0 ) {
                        strcat( buffer, " " );
                        strcat( buffer, s );
                    }
                }
                if( histogram[ index ] ) {
                    ++hits;
                    sfbti_add_entry( histogram[ index ], isequence, ngr_ll_col( ngrf, N ) );
                }
                ++processed;
            }
                fprintf( stderr, "done processing\n" );
            ngr_free(ngrf);
                fprintf( stderr, "done freeing\n" );
        }

        fprintf( stderr, "%lld %d-grams processed (%lld were hits).\n", processed, N, hits );
        
                fprintf( stderr, "done printing message\n" );
        for(long long j=fdno;j<dmax;j++) {
            if( verbose ) {
                fprintf( stderr, "Finalizing SFBT %d of %d.\n", j + 1, dmax );
            }
            if( histogram[j] ) {
                int rv = sfbti_close_wctx( histogram[j] );
                if( rv ) {
                    fprintf( stderr, "fatal error: failure.\n" );
                    return 1;
                }
            }
        }

        fprintf( stderr, "Finished pass %d, covering files %ld to %ld.\n", passno, fdno, dmax-1 );
        passno++;

        fdno = dmax;
    } while( fdno < maxindex );


    free(xs);
    free(histogram);

    free_wordhashes( words );
    
    fprintf( stderr, "Success.\n" );

    return 0;
}