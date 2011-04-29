#include "sfbt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    struct sfbt_record_header my;

    struct sfbt_wctx *wctx = sfbt_new_wctx( "my.output.sfbt" );

    char buffer[ MAX_KEY_SIZE ];

    while(1) {
        if(!fgets( buffer, MAX_KEY_SIZE, stdin )) break;
        char *key = strtok( buffer, "\t" );
        char *vals = strtok( 0, "\n" );
        if( !key || !vals ) {
            fprintf( stderr, "malformed input\n" );
            return 1;
        }
        int64_t val = atoll( vals );
        if(sfbt_add_entry( wctx, buffer,  val )) {
            fprintf( stderr, "fail.\n" );
            return 1;
        }
    }

    if( sfbt_close_wctx( wctx ) ) {
        fprintf(stderr, "failed 2.\n" );
        return 1;
    }

    return 0;
}
