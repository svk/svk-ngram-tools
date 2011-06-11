#include "sfbti.h"
#include <stdio.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int sfbti_check_last_child_gen_at( struct sfbti_wctx* wctx, long foffset ) {
    // are there few enough nodes in this generation that the
    // next node will be the root node?

    do {
        semifile_fpos_t cpos;
        if( semifile_fgetpos( wctx->f, &cpos ) ) break;

        if( semifile_fseek( wctx->f, foffset, SEEK_SET ) ) break;

        int count = 0;

        while( count <= KEYS_PER_RECORD ) {
            struct sfbti_record buffer;
            if( semifile_fread( &buffer, sizeof buffer, 1, wctx->f ) != 1) {
                if( !semifile_feof( wctx->f ) ) return -1;
                semifile_clearerr( wctx->f );
                break;
            }

            count++;
        }

        // yes.
        if( semifile_fsetpos( wctx->f, &cpos ) ) break;
        return count <= KEYS_PER_RECORD;
    } while(0);
    return -1;
}

int sfbti_write_collected_node( struct sfbti_wctx* wctx ) {
    do {
        struct sfbti_record rec;

        memset( &rec, 0, sizeof rec );

        for(int i=0; i < wctx->collected_children; i++) {
            memcpy( rec.keyvals[i], wctx->child_first_key[i], KEY_SIZE ); 

            fprintf( stderr, "copying: " );
            for(int j=0;j<KEY_SIZE;j++) {
                fprintf( stderr, "%02x ", rec.keyvals[i][j] );
            }
            fprintf( stderr, "\n" );

            memcpy( &rec.keyvals[i][KEY_SIZE], &wctx->child_offset[i], OFFSET_SIZE ); 
        }
        rec.entries = wctx->collected_children;
        rec.flags = 0;

        fprintf( stderr, "writing a parent to %08x (%d)\n", semifile_ftell( wctx->f ), wctx->collected_children );

        if( semifile_fwrite( &rec, sizeof rec, 1, wctx->f ) != 1 ) break;

        return 0;
    } while(0);
    return 1;
}

int sfbti_collect_children( struct sfbti_wctx* wctx, long stop_pos ) {
    struct sfbti_record buffer;
    unsigned char *firstkey = (unsigned char*) &buffer.keyvals[0][0];

    do {
        wctx->collected_children = 0;

        while( wctx->collected_children < KEYS_PER_RECORD ) {
            long foffset = semifile_ftell( wctx->f );

            if( foffset >= stop_pos ) break;
                
            if( semifile_fread( &buffer, sizeof buffer, 1, wctx->f ) != 1 ) {
                return 1;
            }

            fprintf( stderr, "collecting a child from %08x (%d), first key: ", foffset, wctx->collected_children );
            for(int i=0;i<KEY_SIZE;i++) {
                fprintf( stderr, "%02x ", firstkey[i] );
            }
            fprintf( stderr, "\n" );

            memcpy( wctx->child_first_key[ wctx->collected_children ], firstkey, KEY_SIZE );
            wctx->child_offset[ wctx->collected_children ] = foffset;

            wctx->collected_children++;
        }
//        fprintf( stderr, "round of child-collecting finished at %d\n", wctx->collected_children );

        return 0;
    } while(0);
    return 1;
}

int sfbti_write_parents( struct sfbti_wctx* wctx ) {
    do {
        long next_gen_foffset = semifile_ftell( wctx->f );

        long last_gen_cur = wctx->current_generation_foffset;
        semifile_fpos_t cur;

        if( semifile_fgetpos( wctx->f, &cur ) ) break;

        if( semifile_fseek( wctx->f, last_gen_cur, SEEK_SET ) ) break;
        if(sfbti_collect_children( wctx, next_gen_foffset )) break;
        last_gen_cur = semifile_ftell( wctx->f );
        if( semifile_fsetpos( wctx->f, &cur ) ) break;
        
        while( wctx->collected_children > 0 ) {
            if(sfbti_write_collected_node( wctx )) return 1;
            if( semifile_fgetpos( wctx->f, &cur ) ) return 1;

            if( semifile_fseek( wctx->f, last_gen_cur, SEEK_SET ) ) return 1;
            if(sfbti_collect_children( wctx, next_gen_foffset )) return 1;
            last_gen_cur = semifile_ftell( wctx->f );
            if( semifile_fsetpos( wctx->f, &cur ) ) return 1;
        }

        wctx->current_generation_foffset = next_gen_foffset;
        return 0;
    } while(0);
    return 1;
}

int sfbti_write_root( struct sfbti_wctx* wctx ) {
    do {
        long t = semifile_ftell( wctx->f );
        if( semifile_fseek( wctx->f, wctx->current_generation_foffset, SEEK_SET ) ) break;
        if(sfbti_collect_children( wctx, t )) break;
        if( !wctx->collected_children ) {
            // should maybe fix this, but it is low-priority because it is not
            // necessary in the binning scheme: if the bin is empty, not
            // ... [logic fail. it is necessary. TODO]
            // [ besides, if I'm experiencing empty bins at all with the hashing
            //   scheme, that's highly likely to be a bug and so a warning is
            //   useful. ]
            fprintf( stderr, "warning: %s is empty and is left invalid\n", wctx->filename );
        } else {
            semifile_rewind( wctx->f );
            if(sfbti_write_collected_node( wctx )) break;
        }

        return 0;
    } while(0);
    return 1;
}

int sfbti_finalize( struct sfbti_wctx* wctx ) {
    if( sfbti_flush_record( wctx ) ) return 1;
    do {
        int rv = sfbti_check_last_child_gen_at( wctx, wctx->current_generation_foffset );
        if( rv < 0 ) return 1;
        if( rv ) break;
        if( sfbti_write_parents( wctx ) ) return 1;
    } while(1);
    if( sfbti_write_root( wctx ) ) return 1;
    return 0;
}

int sfbti_flush_record( struct sfbti_wctx* wctx ) {
    do {
//        if( semifile_fgetpos( wctx->f, &cpos ) ) break;
//        fprintf( stderr, "wctx fpos before flush : %ld\n", cpos );


        fprintf( stderr, "flushing record to %08x, first key: ", semifile_ftell( wctx->f ) );
        for(int i=0;i<KEY_SIZE;i++) {
            fprintf( stderr, "%02x ", wctx->current_record.keyvals[0][i] );
        }
        fprintf( stderr, "\n" );

        for(int i=0;i< wctx->current_record.entries;i++) {
            int nonzero = 0;
            for(int j=0;j<KEY_SIZE;j++) {
                if( wctx->current_record.keyvals[i][j] ) nonzero = 1;
            }
            if( !nonzero ) {
                fprintf( stderr, "record has zero key: %d\n", i );
            }
        }

        if( semifile_fwrite( &wctx->current_record, sizeof wctx->current_record, 1, wctx->f ) != 1 ) break;
//        fprintf( stderr, "[beta]\n" );

//        if( semifile_fsetpos( wctx->f, &cpos ) ) break;
//        fprintf( stderr, "[gamma]\n" );

        memset( &wctx->current_record, 0, sizeof wctx->current_record );


        return 0;
    } while(0);
    return 1;
}

int sfbti_new_leaf_record( struct sfbti_wctx* wctx ) {
    do {
        memset( &wctx->current_record, 0, sizeof wctx->current_record );

        wctx->current_record.entries = 0;
        wctx->current_record.flags = FLAG_ENTRIES_ARE_LEAVES;

        return 0;
    } while(0);
    return 1;
}

int sfbti_add_entry( struct sfbti_wctx* wctx, const int * key, int64_t count) {
    do {

        if( wctx->current_record.entries == KEYS_PER_RECORD ) {
            if( sfbti_flush_record( wctx ) ) break;
            if( sfbti_new_leaf_record( wctx ) ) break;
        }
        assert( wctx->current_record.entries < KEYS_PER_RECORD );

        int i = wctx->current_record.entries++;
        for(int j=0;j<NGRAM_SIZE;j++) {
            // assumes little endian!
            memcpy( &wctx->current_record.keyvals[i][TOKEN_SIZE*j], &key[j], TOKEN_SIZE );
        }
        assert( sizeof count == SUFFIX_SIZE );
        memcpy( &wctx->current_record.keyvals[i][KEY_SIZE], &count, sizeof count );

        return 0;
    } while(0);
    return 1;
}

struct sfbti_wctx *sfbti_new_wctx(const char * filename) {
    struct sfbti_wctx *rv = malloc(sizeof *rv);
    int phase = 0;
    do {
        if( !rv ) break;
        ++phase;

        memset( rv, 0, sizeof *rv );
        ++phase;

        strcpy( rv->filename, filename );
        ++phase;

        rv->f = semifile_fopen( filename, 1 );
        if( !rv->f ) break;
        ++phase;

        if( semifile_fseek( rv->f, sizeof rv->current_record, SEEK_SET ) ) break;
        ++phase;

        rv->current_generation_foffset = semifile_ftell( rv->f );
        ++phase;

        if( sfbti_new_leaf_record( rv ) ) break;
        ++phase;

        return rv;
    } while(0);
    fprintf( stderr, "warning: new_wctx failed at phase %d\n", phase );
    if( rv ) free(rv);
    return 0;
}

int sfbti_close_wctx(struct sfbti_wctx* wctx) {
    int rv = sfbti_finalize( wctx );

    if( wctx->f ) {
        semifile_fclose( wctx->f );
    }
    free( wctx );

    return rv;
}

int sfbti_find_index(struct sfbti_record* buf, const int* key, int exact) {
    int mn = 0, mx = buf->entries - 1;

    fprintf( stderr, "finding index of %d", key[0] );
    for(int i=1;i<NGRAM_SIZE;i++) {
        fprintf( stderr, ":%d", key[i] );
    }
    fprintf( stderr, " (exact? %d)\n", exact );

    while( mn != mx ) {
        int mp = (mn + mx + 1) / 2;
        assert( mn < mx );

        int cr = 0;

        fprintf( stderr, "comparing to %d: ", mp );
        for(int i=0;i<KEY_SIZE;i++) {
            fprintf( stderr, "%02x ", buf->keyvals[mp][i] );
        }
        fprintf( stderr, "\n" );
        
        for(int i=0;i<NGRAM_SIZE;i++) {
            int t = (*(int*)(&buf->keyvals[mp][TOKEN_SIZE*i]) & TOKEN_INT_MASK);
            fprintf( stderr, "comparing to %d, %dth: %d", mp, i, t );
            if( key[i] < t ) {
                fprintf( stderr, "finish: <" );
                // arg-key is earlier than rec-key
                mx = mp - 1;
                break;
            } else if( key[i] > t || (i+1) == NGRAM_SIZE ) {
                fprintf( stderr, "finish: >=" );
                // arg-key is later or equal to rec-key
                mn = mp;
                break;
            }
            fprintf( stderr, "\n" );
        }
        fprintf( stderr, "\n" );
    }
    assert( mn == mx );

    fprintf( stderr, "found [%d == %d]\n", mn, mx );

    if( exact ) {
        for(int i=0;i<NGRAM_SIZE;i++) {
            int t = (*(int*)(&buf->keyvals[mn][TOKEN_SIZE*i]) & TOKEN_INT_MASK);
            if( t != key[i] ) return -1;
        }
    }
    return mn;
}

int sfbti_search(struct sfbti_rctx* rctx, const int* key, int64_t* count_out) {
    struct sfbti_cached_record *cache = rctx->root;
    struct sfbti_record *node = &cache->data;

    if( rctx->suspend ) {
        if( sfbti_desuspend_rctx( rctx ) ) return 1;
    }

    while( !(node->flags & FLAG_ENTRIES_ARE_LEAVES) ) {
        int index = sfbti_find_index( node, key, 0 );
        if( index < 0 ) return 1;
        if( cache && cache->cached[index] ) {
            cache = cache->cached[index];
            node = &cache->data;
        } else {
            uint32_t* fpos = (uint32_t*) &node->keyvals[index][KEY_SIZE];
            if( fseek( rctx->f, *fpos, SEEK_SET ) ) return 1;
            if( sfbti_readnode( rctx, &rctx->buffer ) ) return 1;
            cache = 0;
            node = &rctx->buffer;
        }
    }
    int index = sfbti_find_index( node, key, 1 );
    if( index < 0 ) {
        *count_out = 0;
    } else {
        int64_t* countp = (int64_t*)&node->keyvals[index][KEY_SIZE];
        *count_out = *countp;
    }

    if( rctx->suspend ) {
        if( sfbti_suspend_rctx( rctx ) ) return 1;
    }

    return 0;
}

int sfbti_readnode(struct sfbti_rctx *rctx, struct sfbti_record* node ) {
    int rv = fread( node, sizeof *node, 1, rctx->f );
    if( ferror( rctx->f ) ) {
        return 1;
    }
    clearerr( rctx->f );
    return 0;
}

struct sfbti_cached_record* sfbti_cache_node(struct sfbti_rctx* rctx, int depth) {
    struct sfbti_cached_record *rv = malloc(sizeof *rv);
    do {
        rctx->cached_bytes += sizeof *rv;
        if( !rv ) break;

        memset( rv, 0, sizeof *rv );

        if( sfbti_readnode( rctx, &rv->data ) ) break;

        if( depth > 0 ) {
            int i;
            for(i=0;i<KEYS_PER_RECORD;i++) {
                uint32_t* fpos = (uint32_t*) &rv->data.keyvals[i][KEY_SIZE];
                if( fseek( rctx->f, *fpos, SEEK_SET ) ) break;
                rv->cached[i] = sfbti_cache_node( rctx, depth - 1 );
                if( !rv->cached[i] ) break;
            }
            if( i < KEYS_PER_RECORD ) break;
        }

        return rv;
    } while(0);

    if( rv ) {
        for(int i=0;i<KEYS_PER_RECORD;i++) if( rv->cached[i] ) {
            sfbti_free_cache( rv->cached[i] );
        }
    }

    if( rv ) free(rv);

    return 0;
}

void sfbti_free_cache(struct sfbti_cached_record* cache) {
    for(int i=0;i<KEYS_PER_RECORD;i++) if( cache->cached[i] ) {
        sfbti_free_cache( cache->cached[i] );
    }
    free( cache );
}

int sfbti_open_rctx(const char *filename, struct sfbti_rctx* rctx, int depth) {
    memset( rctx, 0, sizeof *rctx );
    rctx->f = fopen( filename, "rb" );
    if( !rctx->f ) return 1;

    do {
        rctx->root = sfbti_cache_node( rctx, depth );
        if( !rctx->root ) break;

        assert( strlen( rctx->filename ) < MAX_SFBT_FILENAME_LEN );
        strcpy( rctx->filename, filename );

        return 0;
    } while(0);
    if( rctx->root ) {
        sfbti_free_cache( rctx->root );
    }
    fclose( rctx->f );
    return 1;
}

int sfbti_close_rctx(struct sfbti_rctx* rctx) {
    if( rctx->f ) {
        fclose( rctx->f );
        rctx->f = 0;
    }
    return 0;
}

int sfbti_suspend_rctx(struct sfbti_rctx* rctx) {
    rctx->suspend = 1; // note asymmetry!

    if( rctx->f ) {
        fclose( rctx->f );
        rctx->f = 0;
    }

    return 0;
}

int sfbti_desuspend_rctx(struct sfbti_rctx* rctx) {
    if( !rctx->f ) {
        rctx->f = fopen( rctx->filename, "rb" );
        if( !rctx->f ) return 1;
    }

    return 0;
}

#ifdef TEST
int main(int argc, char *argv[]) {
    const int N = 1000000;

    struct sfbti_record my;
    fprintf( stderr, "%d\n", sizeof my );

    struct sfbti_wctx *wctx = sfbti_new_wctx( "my.test.sfbti" );
    assert( wctx );
    for(int i=0;i<N;i++) {
        int vals[NGRAM_SIZE];
        int v = i;
        for(int j=0;j<NGRAM_SIZE;j++) {
            vals[NGRAM_SIZE-j-1] = v % 25;
            v /= 25;
        }
        if( (i%100000) == 0 ) {
            fprintf( stderr, "adding %d: %d, %d, %d, %d, %d\n", i, vals[0], vals[1], vals[2], vals[3], vals[4] );
        }
        if( sfbti_add_entry( wctx, vals, i + 1 ) ) {
            return 1;
        }
    }
    fprintf( stderr, "closing wctx\n" );
    if( sfbti_close_wctx( wctx ) ) {
        fprintf(stderr, "failed 2.\n" );
        return 1;
    }
    fprintf( stderr, "done writing\n" );

    struct sfbti_rctx rctx;
    if( sfbti_open_rctx( "my.test.sfbti", &rctx, 1 ) ) {
        fprintf( stderr, "readfail\n" );
        return 1;
    }
    fprintf( stderr, "Spending %d bytes on cache.\n", rctx.cached_bytes );

    fprintf( stderr, "Beginning reads.\n" );
    for(int i=0;i<N;i++) {
        int vals[NGRAM_SIZE];
        int v = i;
        for(int j=0;j<NGRAM_SIZE;j++) {
            vals[NGRAM_SIZE-j-1] = v % 25;
            v /= 25;
        }
        int64_t count;
        int rv = sfbti_search( &rctx, vals, &count );
        if( count != i + 1 ) {
            fprintf( stderr, "oops (%d). %d\n", i, rv );
            return 1;
        }
    }
    fprintf( stderr, "Finished reads.\n" );

    if( sfbti_close_rctx( &rctx ) ) {
        fprintf( stderr, "readclosefail\n" );
        return 1;
    }
    
    return 0;
}
#endif
