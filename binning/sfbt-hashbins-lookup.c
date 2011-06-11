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
    char *IDN = 0;
    int prefix_digits = 0;
    char *suffix = ".sfbt";

    while(1) {
        int c = getopt( argc, argv, "vcn:o:B:p:U:I:V:" );
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
            case 'I':
                IDN = optarg;
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

    if( !IDN ) {
        fprintf( stderr, "fatal error: input argument -I (input directory) missing\n" );
        return 1;
    }


    struct sfbt_rctx *histogram = 0;
    long long memreq = sizeof *histogram * ipow( no_bins, N );
    long long maxindex = ipow( no_bins, N );
    int *xs = malloc(sizeof *xs * N );
    assert( xs );

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
            fprintf( stderr, "\tLoading:\n" );
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
        strcpy( filename, IDN );
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
        int rv = sfbt_open_rctx( filename, &histogram[j], 0 );
        if( rv || sfbt_suspend_rctx( &histogram[j] ) ) {
            fprintf( stderr, "fatal error: load/suspend error on %s or out of memory\n", filename );
            return 1;
        }
    }

    long long processed = 0;

    char buffer[ MAX_KEY_SIZE ], keybuf[ MAX_KEY_SIZE ];

    fprintf( stderr, "Ready for lookups.\n" );

    while(1) {
        if(!fgets( buffer, MAX_KEY_SIZE, stdin )) break;
        char *key = strtok( buffer, "\t\n" );
        strcpy( keybuf, key );
        char *colkey = strtok( buffer, " \t" );
        int index = 0;
        int i;
        for(i=0;i<N;i++) {
            if( !colkey ) break;
			int n = classify_uint32( no_bins, murmur_hash( colkey, strlen(colkey) ) );
            index *= no_bins;
            index += n;
#if 0
			if( verbose ) {
				fprintf( stderr, "Classified \"%s\" as %d\n", colkey, n );
			}
#endif
            colkey = strtok( 0, " \t" );
        }
        if( i != N ) {
            fprintf( stderr, "warning: input \"%s\" invalid (not enough columns), ignoring\n", keybuf );
            continue;
        }
		if( verbose ) {
			fprintf( stderr, "Classified \"%s\" as %d (%s)\n", keybuf, index, histogram[index].filename );
		}
        int64_t count;
        int rv = sfbt_search( &histogram[index], keybuf, &count );
        if( rv ) {
            fprintf( stderr, "warning: lookup error on \"%s\"\n", keybuf );
        } else {
            fprintf( stdout, "%s\t%lld\n", keybuf, count );
        }
        ++processed;
    }

    if( verbose ) {
        fprintf( stderr, "%lld queries processed.\n", processed );
    }

    for(long long j=0;j<maxindex;j++) {
		sfbt_close_rctx( &histogram[j] );
	}

    free(histogram);

    return 0;
}
