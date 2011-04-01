#include "bins.h"

#include <assert.h>
#include <string.h>

int main(int argc, char *argv[]) {
    const int B = 3;
    const int K = 1000;
    const char *FN = "bintable_test.out";
    const long long TW = ((1+K)*K)/2;
    char buf[1024];
    struct bin_cons *bc = make_bincons( TW, B );
    int i;

    assert( bc );

    for(i=1;i<=1000;i++) {
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
