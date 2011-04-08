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
 * (Numbers for cache include cache construction time, so they are actually
 * slightly faster in the long run. 300k lookups were done for each
 * experiment -- hitting each entry once.)
 *
 * Caching the root node only, with all uncompressed: 23 076 lookups
 * per second, still mostly disk bound, reduction is mostly disk activity.
 * Cache memory usage ~ 32 kilobytes.
 *
 * Caching the at depth 1, with all uncompressed: 39 473 lookups
 * per second, still _mostly_ disk bound, reduction is mostly disk activity
 * but also some CPU.
 * Cache memory usage ~ 4 megabytes.
 *
 * Caching the at depth 2, with all uncompressed: 330 033 lookups
 * per second. The tree is of height 2, so this eliminates disk activity
 * during lookups. Note that we are still including cache construction time,
 * including all the disk reads. 
 * Cache memory usage ~ 516 megabytes (actually less since the entire tree
 * is cached).
 *
 * The realistic model for bins is probably somewhere between depth 0 and
 * depth 1 (caching the root node for all nodes, caching more for large
 * bins). Some bins will be of height 2 but with lots of null nodes, these
 * might be cached entirely just because it's approximately as cheap
 * as caching depth 0.
 *
 * The realistic model for one huge B-tree is depth 2 on a normal computer
 * and depth 3 (64 gigabytes) on a larger machine. Note that at depth 3 this
 * method might be less than ideal, as this is actually more than enough
 * enough memory to hold the entire compressed corpus in memory, but not
 * nearly enough to hold the entire uncompressed, alignment-padded corpus.
 */

#ifdef DEBUG_LOOKUP
#include <stdio.h>
#endif

#ifdef SUPPORT_COMPRESSION
#include <zlib.h>
#define BTREE_FOPEN gzopen
#define BTREE_FILE gzFile
#define BTREE_FEOF gzeof
#define BTREE_FCLOSE gzclose
#else
#include <stdio.h>
#define BTREE_FILE FILE*
#define BTREE_FOPEN fopen
#define BTREE_FEOF feof
#define BTREE_FCLOSE fclose
#endif

int btree_read_file( BTREE_FILE f, char* records) {
    int bytesRead = 0;
    int rrv;

    do {
#ifdef SUPPORT_COMPRESSION
        rrv = gzread( f, &records[bytesRead], RECORD_SIZE * MAX_RECORDS - bytesRead );
#else
        rrv = fread( &records[bytesRead], 1, RECORD_SIZE * MAX_RECORDS - bytesRead, f );
#endif
        if( !rrv ) {
            if( !BTREE_FEOF(f) ) {
                return -1;
            }
            break;
        }
        bytesRead += rrv;
    } while( bytesRead < RECORD_SIZE * MAX_RECORDS );
    assert( bytesRead % RECORD_SIZE == 0 );

    return bytesRead / RECORD_SIZE;
}

void btree_free_cache( struct btree_cached_record* cache ) {
    for(int i=0;i<cache->n;i++) if( cache->cache_next[i] ) {
        btree_free_cache( cache->cache_next[i] );
    }
    free( cache );
}

struct btree_cached_record* btree_make_cache( const char *prefix, const char *nodename, int depth ) {
    struct btree_cached_record *rv = malloc(sizeof *rv);
    char *filename = alloca( strlen(prefix) + strlen(nodename) + 1 );

    strcpy( filename, prefix );
    strcat( filename, "/" );
    strcat( filename, nodename );

    if( rv ) {
        memset( rv, 0, sizeof *rv );

        BTREE_FILE f = BTREE_FOPEN( filename, "rb" );
        if( !f || (rv->n = btree_read_file( f, (char*) &rv->data ) ) < 0 ) {
            fprintf( stderr, "fatal error: unable to allocate cache (file input error on %s)\n", nodename );
            if( f ) BTREE_FCLOSE( f );
            free( rv );
            return 0;
        }
        BTREE_FCLOSE( f );

        if( depth > 0 ) {
            int i;
            for(i=0;i<rv->n;i++) {
                const char *fn = rv->data[i].filename;
                if( strlen(fn) > 0 ) {
                    rv->cache_next[i] = btree_make_cache( prefix, fn, depth - 1 );
                    if( !rv->cache_next[i] ) {
                        while( --i >= 0 ) {
                            btree_free_cache( rv->cache_next[i] );
                        }
                        free( rv );
                        return 0;
                    }
                }
            }
        }
    } else {
        fprintf( stderr, "fatal error: unable to allocate cache (out of memory)\n" );
    }

    return rv;
}

int64_t btree_lookup(struct btree_cached_record* cache, const char *prefix, const char *token) {
    const int prefixlen = strlen(prefix);
    const int filenamesize = prefixlen + 1 + FILENAME_SIZE;
    int rv = 0;
    int ok = 0;


    char *filename = alloca( filenamesize );
    struct btree_record* localbuf = malloc( sizeof *localbuf * MAX_RECORDS );
    if( !localbuf ) {
        return -1;
    }

    struct btree_record* records = 0;

    memset( filename, 0, prefixlen );
    strcpy( filename, prefix );
    strcat( filename, "/root" );
    char *tailname = &filename[ prefixlen + 1 ];

    assert( sizeof *localbuf == RECORD_SIZE );

    do {
        int bytesRead = 0, rrv, recordsRead;

        if( cache ) {
#ifdef DEBUG_LOOKUP
            fprintf( stderr, "lookup: opening cached node %p\n", cache );
#endif
            recordsRead = cache->n;
            records = cache->data;
        } else {
            records = localbuf;

#ifdef DEBUG_LOOKUP
            fprintf( stderr, "lookup: opening \"%s\"\n", tailname );
#endif
            BTREE_FILE f = BTREE_FOPEN( filename, "rb" );
            if( !f ) {
                if( !strcmp( tailname, "root") ) {
                    rv = 0;
                } else {
                    rv = -1;
                }
                break;
            }

            recordsRead = btree_read_file( f, (char*) records );

#ifdef DEBUG_LOOKUP
            fprintf( stderr, "lookup: read %d records\n", recordsRead );
#endif
            BTREE_FCLOSE(f);

            if( recordsRead < 0 ) {
                rv = -1;
                break;
            }
        }


        int mn = 0, mx = recordsRead - 1;
        int debug_loops = 0;

        while( mn != mx ) {
            int mp = (mn + mx + 1) / 2;
            assert( mn < mx );

            int cr = strcmp( token, records[mp].token );
#ifdef DEBUG_LOOKUP
        fprintf( stderr, "lookup: [\"%s\" -- \"%s\"] [%d, %d, %d] -> %d\n", token, records[mp].token, mn, mp, mx, cr );
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
            if( cache ) {
                cache = cache->cache_next[mn];
            }
            if( !cache ) {
                strcpy( tailname, records[mn].filename );
            }
#ifdef DEBUG_LOOKUP
            fprintf( stderr, "lookup: next file is \"%s\"\n", tailname);
#endif
        }

#ifdef DEBUG_LOOKUP
            fprintf( stderr, "ok: %d\n", ok);
#endif
    } while(!ok);

#ifdef DEBUG_LOOKUP
            fprintf( stderr, "goodbye!\n" );
#endif

    free( localbuf );
    return rv;
#undef BTREE_LOOKUP_FCLOSE
}
