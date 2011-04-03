#include "bins.h"
#include "ngread.h"

#include "freegetopt-0.11/getopt.h"

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <string.h>

int main(int argc, char* argv[]) {
    int B = 10;
    char *OFN = "bintable_mkbins.out";
    int verbose = 0;
    long long W = -1;
    char *IFN = 0;
    int verify_order = 0;
    int find_size = 0;
    while(1) {
        int c = getopt( argc, argv, "vb:o:W:VS" );
        if( c < 0 ) break;
        switch( c ) {
            case 'S':
                find_size = 1;
                break;
            case 'V':
                verify_order = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'b':
                B = atoi( optarg );
                break;
            case 'W':
                W = atoll( optarg );
                break;
            case 'o':
                OFN = optarg;
                break;
            default:
                fprintf(stderr, "unknown option: '%c'\n", c );
                exit(1);
        }
    }
    int argi = optind;
    if( argi < argc ) {
        IFN = argv[argi++];
    }
    if( !IFN ) {
        fprintf( stderr, "fatal error: input argument (vocabulary file) missing\n" );
        return 1;
    }
    if( W < 0 ) {
        fprintf( stderr, "fatal error: input argument -W (expected total count) missing\n" );
        return 1;
    }

    if( verbose ) {
        fprintf( stderr, "Parameters:\n" );
        fprintf( stderr, "\tb: %d bins\n", B );
        fprintf( stderr, "\tW: %lld total count\n", W );
        fprintf( stderr, "\to: output to \"%s\"\n", OFN );
        fprintf( stderr, "\tinput from \"%s\"\n", IFN );
    }

    FILE *of = fopen( OFN, "wb" );
    assert( of );

    struct bin_cons *bc = make_bincons( W, B );
    assert( bc );

    int largest_size = -1;;

    char *last = 0;
    int lastlen = 8;

    struct ngr_file *ngrf = ngr_open( IFN );
    while( ngr_next(ngrf) ) {
        assert( ngr_columns(ngrf) == 2 );
        if( find_size ){
            int l = strlen( ngr_s_col( ngrf, 0 ) );
            largest_size = (largest_size > l) ? largest_size : l;
        }
        if( verify_order ) {
            if( last ) {
                if( strcmp( last, ngr_s_col( ngrf, 0 ) ) >= 0 ) {
                    fprintf( stderr, "verification failed: \"%s\" occurs before \"%s\"\n", last, ngr_s_col(ngrf,0) );
                    exit(1);
                }
            }
            int reqbuflen = strlen( ngr_s_col( ngrf, 0 ) ) + 1;
            int mrealloc = !last;
            while( reqbuflen > lastlen ) {
                lastlen *= 2;
                mrealloc = 1;
            }
            if( mrealloc ) {
                if( last ) {
                    free( last );
                }
                last = malloc( lastlen );
                strcpy( last, ngr_s_col( ngrf, 0 ) );
            }
        }
        int err = update_bincons( bc, ngr_s_col(ngrf,0), ngr_ll_col(ngrf,1) );
        assert( !err );
    }
    ngr_free(ngrf);

    bincons_write( bc, of );

    fclose(of);

    if( last ) {
        free( last );
    }

    if( find_size ) {
        fprintf( stderr, "info: longest token was %d bytes long.\n", largest_size );
    }

    return 0;
}
