#include "bins.h"
#include "ngread.h"

#include "freegetopt-0.11/getopt.h"

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

int main(int argc, char* argv[]) {
    int B = 10;
    char *OFN = "bintable_mkbins.out";
    int verbose = 0;
    long long W = -1;
    char *IFN = 0;
    while(1) {
        int c = getopt( argc, argv, "vb:o:W:" );
        if( c < 0 ) break;
        switch( c ) {
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

    struct ngr_file *ngrf = ngr_open( IFN );
    while( ngr_next(ngrf) ) {
        assert( ngr_columns(ngrf) == 2 );
        int err = update_bincons( bc, ngr_s_col(ngrf,0), ngr_ll_col(ngrf,1) );
        assert( !err );
    }
    ngr_free(ngrf);

    bincons_write( bc, of );

    fclose(of);

    return 0;
}
