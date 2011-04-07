#include "btreelu.h"

#include <assert.h>

int64_t btree_lookup(const char *prefix, const char *token) {
    const int prefixlen = strlen(prefix);
    const int filenamesize = prefixlen + 1 + FILENAME_SIZE;
    char *filename = alloca( filenamesize );
    struct btree_record* records = malloc( sizeof *records * MAX_RECORDS );
    if( !records ) {
        return -1;
    }

    memset( filename, 0, prefixlen );
    strcpy( filename, prefix );
    strcat( filename, "/root" );
    char *tailname = &filename[ prefixlen + 1 ];

    assert( sizeof *records == RECORD_SIZE );

    do {
        int bytesRead = 0, rrv, recordsRead;
        gzFile f = gzopen( filename, "rb" );

        do {
            rrv = gzread( f, &records[bytesRead], RECORD_SIZE * MAX_RECORDS - bytesRead );
            if( !rrv ) {
                if( gzerror(f) ) {
                    gzclose(f);
                    free( records );
                    return -1;
                }
                break;
            }
            rrv += bytesRead;
        } while(1);
        recordsRead = bytesRead / RECORD_SIZE;

        int mn = 0, mx = recordsRead - 1;

        // insert binary search here

        gzclose( f );
    } while(1);

    free( records );
    return 0;
}
