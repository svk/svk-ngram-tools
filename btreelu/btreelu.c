#include "btreelu.h"

#include <assert.h>
#include <stdint.h>

#include <string.h>
#include <stdlib.h>

#include <alloca.h>
#include <zlib.h>

#define DEBUG_LOOKUP

#ifdef DEBUG_LOOKUP
#include <stdio.h>
#endif

int64_t btree_lookup(const char *prefix, const char *token) {
    const int prefixlen = strlen(prefix);
    const int filenamesize = prefixlen + 1 + FILENAME_SIZE;
    int rv = 0;
    int ok = 0;

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
        int rv;
        int ok = 0;

        gzFile f = gzopen( filename, "rb" );
        if( !f ) {
            if( !strcmp( tailname, "root") ) {
                rv = 0;
            } else {
                rv = -1;
            }
            gzclose(f);
            break;
        }
#ifdef DEBUG_LOOKUP
        fprintf( stderr, "lookup: opened \"%s\"\n", tailname );
#endif

        do {
            rrv = gzread( f, &records[bytesRead], RECORD_SIZE * MAX_RECORDS - bytesRead );
            if( !rrv ) {
                if( !gzeof(f) ) {
                    gzclose(f);
                    free( records );
                    return -1;
                }
                break;
            }
            rrv += bytesRead;
        } while(1);
        assert( recordsRead % RECORD_SIZE == 0 );

        recordsRead = bytesRead / RECORD_SIZE;
#ifdef DEBUG_LOOKUP
        fprintf( stderr, "lookup: read %d records\n", recordsRead );
#endif

        int mn = 0, mx = recordsRead - 1;
        int debug_loops = 0;

        while( mn != mx ) {
            int mp = (mn + mx + 1) / 2;
            assert( mn < mx );

            int cr = strcmp( token, records[mp].token );
#ifdef DEBUG_LOOKUP
        fprintf( stderr, "lookup: [%d, %d, %d] -> %d\n", mn, mp, mx, cr );
#endif

            if( cr < 0 ) {
                mx = mp - 1;
            } else if( cr >= 0 ) {
                mn = mp;
            }

            assert( ++debug_loops < 1000 );
        }
        assert( mn == mx );

#ifdef DEBUG_LOOKUP
        fprintf( stderr, "lookup: classified as %d\n", mn );
#endif

        if( !strcmp( token, records[mn].token ) ) {
#ifdef DEBUG_LOOKUP
            fprintf( stderr, "lookup: exact match, returning\n");
#endif
            rv = records[mn].weight;
            ok = 1;
        } else if( records[mn].filename[0] == '\0' ) {
#ifdef DEBUG_LOOKUP
            fprintf( stderr, "lookup: dead end, returning zero\n");
#endif
            rv = 0;
            ok = 1;
        } else {
            strcpy( tailname, records[mn].filename );
#ifdef DEBUG_LOOKUP
            fprintf( stderr, "lookup: next file is \"%s\"\n", tailname);
#endif
        }

        gzclose( f );
    } while(!ok);

    free( records );
    return rv;
}
