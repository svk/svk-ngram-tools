#include "btreelu.h"

#include <assert.h>
#include <stdint.h>

#include <string.h>
#include <stdlib.h>

#include <alloca.h>

/* Defining SUPPORT_COMPRESSION uses zlib which autodetects whether
 * a file is compressed or not. However, this uses a little CPU,
 * and without SUPPORT_COMPRESSION the lookup uses very little CPU,
 * so it's wise to leave this undefined if none of the files
 * actually are compressed.
 *
 * From my tests, with all files compressed the lookup is almost completely
 * CPU bound and runs at about 893 lookups per second, with all files
 * uncompressed it is almost completely disk bound and runs at about
 * 11 538 lookups per second, both with a total set size of about 300k
 * entries.
 *
 * No experiments on cached nodes yet, not even for the root node.
 */

#ifdef DEBUG_LOOKUP
#include <stdio.h>
#endif

#ifdef SUPPORT_COMPRESSION
#include <zlib.h>
#else
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

#ifdef SUPPORT_COMPRESSION
        gzFile f = gzopen( filename, "rb" );
#define BTREE_LOOKUP_FEOF gzeof
#define BTREE_LOOKUP_FCLOSE gzclose
#else
        FILE *f = fopen( filename, "rb" );
#define BTREE_LOOKUP_FEOF feof
#define BTREE_LOOKUP_FCLOSE fclose
#endif
        if( !f ) {
            if( !strcmp( tailname, "root") ) {
                rv = 0;
            } else {
                rv = -1;
            }
            BTREE_LOOKUP_FCLOSE(f);
            break;
        }
#ifdef DEBUG_LOOKUP
        fprintf( stderr, "lookup: opened \"%s\"\n", tailname );
#endif

        do {
#ifdef SUPPORT_COMPRESSION
            rrv = gzread( f, &records[bytesRead], RECORD_SIZE * MAX_RECORDS - bytesRead );
#else
            rrv = fread( &records[bytesRead], 1, RECORD_SIZE * MAX_RECORDS - bytesRead, f );
#endif
            if( !rrv ) {
                if( !BTREE_LOOKUP_FEOF(f) ) {
                    BTREE_LOOKUP_FCLOSE(f);
                    free( records );
                    return -1;
                }
                break;
            }
            bytesRead += rrv;
        } while( bytesRead < RECORD_SIZE * MAX_RECORDS );
        assert( bytesRead % RECORD_SIZE == 0 );

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
            fprintf( stderr, "lookup: exact match %llu, returning\n", records[mn].weight);
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

        BTREE_LOOKUP_FCLOSE( f );

#ifdef DEBUG_LOOKUP
            fprintf( stderr, "ok: %d\n", ok);
#endif
    } while(!ok);

#ifdef DEBUG_LOOKUP
            fprintf( stderr, "goodbye!\n" );
#endif

    free( records );
    return rv;
#undef BTREE_LOOKUP_FCLOSE
}
