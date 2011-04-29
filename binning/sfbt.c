#include "sfbt.h"
#include <stdio.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int sfbt_check_last_child_gen_at( struct sfbt_wctx* wctx, long foffset ) {
    // are there few enough nodes in this generation that the
    // next node will be the root node?

    do {
        fpos_t cpos;
        if( fgetpos( wctx->f, &cpos ) ) break;

        if( fseek( wctx->f, foffset, SEEK_SET ) ) break;

        int count = 0;

        while( count <= KEYS_PER_RECORD ) {
            uint32_t recsize;
            if( fread( &recsize, sizeof recsize, 1, wctx->f ) != 1) {
                if( !feof( wctx->f ) ) return -1;
                clearerr( wctx->f );
                break;
            }

            if( fseek( wctx->f, recsize - 4, SEEK_CUR ) ) break;

            count++;
        }

        // yes.
        if( fsetpos( wctx->f, &cpos ) ) break;
        return count <= KEYS_PER_RECORD;
    } while(0);
    return -1;
}

int sfbt_write_collected_node( struct sfbt_wctx* wctx ) {
    do {
        struct sfbt_record_header hdr;

        memset( &hdr, 0, sizeof hdr );
        hdr.record_size = RECORD_HEADER_SIZE;
        hdr.entries = 0;

        for(int i=0; i < wctx->collected_children; i++) {
            hdr.entry_offsets[ hdr.entries++ ] = hdr.record_size;
            hdr.record_size += 1 + strlen( wctx->child_first_key[i] ) + ENTRY_SUFFIX_SIZE;
        }

        hdr.entries_are_leaves = 0;

        if( fwrite( &hdr, sizeof hdr, 1, wctx->f ) != 1 ) break;

        int i;
        for(i=0; i < wctx->collected_children; i++) {
            union sfbt_entry_suffix suff;
            memset( &suff, 0, sizeof suff );
            suff.offset = wctx->child_offset[i];

            if( fwrite( wctx->child_first_key[i], strlen( wctx->child_first_key[i] ) + 1, 1, wctx->f ) != 1 ) break;
            if( fwrite( &suff, sizeof suff, 1, wctx->f ) != 1 ) break;
        }
        if( i < wctx->collected_children ) break; 

        return 0;
    } while(0);
    return 1;
}

int sfbt_collect_children( struct sfbt_wctx* wctx, long stop_pos ) {
    unsigned char buffer[ MAX_RECORD_SIZE ];
    uint32_t *recsize = (uint32_t*) buffer;
    char *firstkey = (char*) &buffer[ RECORD_HEADER_SIZE ];

    do {
        wctx->collected_children = 0;

        while( wctx->collected_children < KEYS_PER_RECORD ) {
            long foffset = ftell( wctx->f );

            if( foffset >= stop_pos ) break;
                
            if( fread( &buffer[0], 4, 1, wctx->f ) != 1 ) {
                return 1;
            }
            if( fread( &buffer[4], *recsize - 4, 1, wctx->f ) != 1 ) {
                return 1;
            }

            assert( strlen( firstkey ) < MAX_KEY_SIZE );
            strcpy( wctx->child_first_key[ wctx->collected_children ], firstkey );
            wctx->child_offset[ wctx->collected_children ] = foffset;

            wctx->collected_children++;
        }

        return 0;
    } while(0);
    return 1;
}

int sfbt_write_parents( struct sfbt_wctx* wctx ) {
    do {
        long next_gen_foffset = ftell( wctx->f );

        long last_gen_cur = wctx->current_generation_foffset;
        fpos_t cur;

        if( fgetpos( wctx->f, &cur ) ) break;

        if( fseek( wctx->f, last_gen_cur, SEEK_SET ) ) break;
        if(sfbt_collect_children( wctx, next_gen_foffset )) break;
        last_gen_cur = ftell( wctx->f );
        if( fsetpos( wctx->f, &cur ) ) break;
        
        while( wctx->collected_children > 0 ) {
            if(sfbt_write_collected_node( wctx )) return 1;
            if( fgetpos( wctx->f, &cur ) ) return 1;

            if( fseek( wctx->f, last_gen_cur, SEEK_SET ) ) return 1;
            if(sfbt_collect_children( wctx, next_gen_foffset )) return 1;
            last_gen_cur = ftell( wctx->f );
            if( fsetpos( wctx->f, &cur ) ) return 1;
        }

        wctx->current_generation_foffset = next_gen_foffset;
        return 0;
    } while(0);
    return 1;
}

int sfbt_write_root( struct sfbt_wctx* wctx ) {
    do {
        long t = ftell( wctx->f );
        if( fseek( wctx->f, wctx->current_generation_foffset, SEEK_SET ) ) break;
        if(sfbt_collect_children( wctx, t )) break;
        if( !wctx->collected_children ) {
            // should maybe fix this, but it is low-priority because it is not
            // necessary in the binning scheme: if the bin is empty, not
            // ... [logic fail. it is necessary. TODO]
            // [ besides, if I'm experiencing empty bins at all with the hashing
            //   scheme, that's highly likely to be a bug and so a warning is
            //   useful. ]
            fprintf( stderr, "warning: %s is empty and is left invalid\n", wctx->filename );
        } else {
            rewind( wctx->f );
            if(sfbt_write_collected_node( wctx )) break;
        }

        return 0;
    } while(0);
    return 1;
}

int sfbt_finalize( struct sfbt_wctx* wctx ) {
    if( sfbt_flush_record( wctx ) ) return 1;
    do {
        int rv = sfbt_check_last_child_gen_at( wctx, wctx->current_generation_foffset );
        if( rv < 0 ) return 1;
        if( rv ) break;
        if( sfbt_write_parents( wctx ) ) return 1;
    } while(1);
    if( sfbt_write_root( wctx ) ) return 1;
    return 0;
}

int sfbt_flush_record( struct sfbt_wctx* wctx ) {
    do {
        fpos_t cpos;
        if( fgetpos( wctx->f, &cpos ) ) break;

        if( fsetpos( wctx->f, &wctx->current_header_pos ) ) break;

        assert( sizeof wctx->current_header == RECORD_HEADER_SIZE );

        wctx->current_header.record_size = wctx->local_offset;

        if( fwrite( &wctx->current_header, RECORD_HEADER_SIZE, 1, wctx->f ) != 1 ) break;

        if( fsetpos( wctx->f, &cpos ) ) break;

        memset( &wctx->current_header, 0, sizeof wctx->current_header );
        wctx->local_offset = 0;

        return 0;
    } while(0);
    return 1;
}

int sfbt_new_leaf_record( struct sfbt_wctx* wctx ) {
    do {
        if( fgetpos( wctx->f, &wctx->current_header_pos ) ) break;

        memset( &wctx->current_header, 0, sizeof wctx->current_header );
        if( fwrite( &wctx->current_header, RECORD_HEADER_SIZE, 1, wctx->f ) != 1 ) break;

        wctx->current_header.entries = 0;
        wctx->current_header.entries_are_leaves = 1;

        wctx->local_offset = RECORD_HEADER_SIZE;

        return 0;
    } while(0);
    return 1;
}

int sfbt_desuspend_wctx( struct sfbt_wctx* wctx ) {
    if( !wctx->f ) {
        wctx->f = fopen( wctx->filename, "rb+" );
        if( !wctx->f ) return 1;
        if( fseek( wctx->f, wctx->current_pos, SEEK_SET ) ) return 1;
    }
    return 0;
}

int sfbt_suspend_wctx( struct sfbt_wctx* wctx ) {
    wctx->current_pos = ftell( wctx->f );
    if( fclose( wctx->f ) ) return 1;
    wctx->f = 0;
    return 0;
}

int sfbt_add_entry( struct sfbt_wctx* wctx, const char * key, int64_t count) {
    do {
#ifdef DEBUG
        assert( strcmp( wctx->debug_last_key, key ) < 0 );
        strcpy( wctx->debug_last_key, key );
#endif

        if( sfbt_desuspend_wctx( wctx ) ) break;

        if( wctx->current_header.entries == KEYS_PER_RECORD ) {
            if( sfbt_flush_record( wctx ) ) break;
            if( sfbt_new_leaf_record( wctx ) ) break;
        }
        assert( wctx->current_header.entries < KEYS_PER_RECORD );

        const int stringsize = strlen(key) + 1;
        static const char nulls[] = {0,0,0,0,0,0,0,0};

        if( fwrite( key, stringsize, 1, wctx->f ) != 1 ) break;

        union sfbt_entry_suffix suff;
        memset( &suff, 0, sizeof suff );
        suff.count = count;
        assert( sizeof suff == ENTRY_SUFFIX_SIZE );
        if( fwrite( &suff, sizeof suff, 1, wctx->f ) != 1 ) break;
        const int corr = (KEY_ALIGNMENT - ((stringsize + COUNT_SIZE) % KEY_ALIGNMENT)) % KEY_ALIGNMENT;
        if( corr ) {
            assert( corr > 0 );
            if( fwrite( nulls, corr, 1, wctx->f ) != 1 ) break;
        }

        wctx->current_header.entry_offsets[ wctx->current_header.entries++ ] = wctx->local_offset;

        const int written = stringsize + COUNT_SIZE + corr;
        wctx->local_offset += written;

        if( sfbt_suspend_wctx( wctx ) ) break;

        return 0;
    } while(0);
    return 1;
}

struct sfbt_wctx *sfbt_new_wctx(const char * filename) {
    struct sfbt_wctx *rv = malloc(sizeof *rv);
    int phase = 0;
    do {
        if( !rv ) break;
        ++phase;

        memset( rv, 0, sizeof *rv );
        ++phase;

        strcpy( rv->filename, filename );
        ++phase;

        rv->f = fopen( filename, "wb+" );
        if( !rv->f ) break;
        ++phase;

        if( fseek( rv->f, MAX_RECORD_SIZE, SEEK_SET ) ) break;
        ++phase;

        rv->current_generation_foffset = ftell( rv->f );
        ++phase;
        if( fgetpos( rv->f, &rv->current_header_pos) ) break;
        ++phase;

        if( sfbt_new_leaf_record( rv ) ) break;
        ++phase;

        if( sfbt_suspend_wctx( rv ) ) break;
        ++phase;

        return rv;
    } while(0);
    fprintf( stderr, "warning: new_wctx failed at phase %d\n", phase );
    if( rv ) free(rv);
    return 0;
}

int sfbt_close_wctx(struct sfbt_wctx* wctx) {
    if( sfbt_desuspend_wctx( wctx ) ) {
        return 1;
    }

    int rv = sfbt_finalize( wctx );

    if( wctx->f ) {
        fclose( wctx->f );
    }
    free( wctx );

    return rv;
}

int sfbt_find_index(union sfbt_record_buffer* buf, const char* key, int exact) {
    int mn = 0, mx = buf->header.entries - 1;

    while( mn != mx ) {
        int mp = (mn + mx + 1) / 2;
        assert( mn < mx );

        const char *that_key = (const char*) &buf->raw[ buf->header.entry_offsets[mp] ];

        int cr = strcmp( key, that_key );
        if( cr < 0 ) {
#ifdef DEBUG
            fprintf( stderr, "\"%s\" is earlier than \"%s\"\n", key, that_key );
#endif
            mx = mp - 1;
        } else if ( cr >= 0 ) {
#ifdef DEBUG
            fprintf( stderr, "\"%s\" is later or equal to \"%s\"\n", key, that_key );
#endif
            mn = mp;
        }
    }
    assert( mn == mx );

    char *that_entry = (char*) &buf->raw[ buf->header.entry_offsets[mn] ];
#ifdef DEBUG
    fprintf( stderr, "\"%s\" corresponds to \"%s\"\n", key, that_entry );
#endif
    if( exact && strcmp( key, that_entry ) ) {
        return -1;
    }
    return mn;
}

int sfbt_search(struct sfbt_rctx* rctx, const char* key, int64_t* count_out) {
    struct sfbt_cached_record *cache = rctx->root;
    union sfbt_record_buffer *node = (union sfbt_record_buffer*) cache->data;

    if( rctx->suspend ) {
        if( sfbt_desuspend_rctx( rctx ) ) return 1;
    }

    while( !node->header.entries_are_leaves ) {
        int index = sfbt_find_index( (union sfbt_record_buffer*) node, key, 0 );
        if( index < 0 ) return 1;
        if( cache && cache->cached[index] ) {
            cache = cache->cached[index];
            node = (union sfbt_record_buffer*) cache->data;
        } else {
            const char *s = &node->raw[ node->header.entry_offsets[index] ];
            uint32_t* fpos = (uint32_t*) &s[strlen(s)+1];
            if( fseek( rctx->f, *fpos, SEEK_SET ) ) return 1;
            if( sfbt_readnode( rctx, &rctx->buffer ) ) return 1;
            cache = 0;
            node = &rctx->buffer;
        }
    }
    int index = sfbt_find_index( node, key, 1 );
    if( index < 0 ) {
        *count_out = 0;
    } else {
        const char *s = &node->raw[ node->header.entry_offsets[index] ];
        int64_t* countp = (int64_t*) &s[strlen(s)+1];
        *count_out = *countp;
    }

    if( rctx->suspend ) {
        if( sfbt_suspend_rctx( rctx ) ) return 1;
    }

    return 0;
}

int sfbt_readnode(struct sfbt_rctx *rctx, union sfbt_record_buffer* node ) {
    int rv = fread( node->raw, 1, MAX_RECORD_SIZE, rctx->f );
    if( ferror( rctx->f ) ) {
        return 1;
    }
    clearerr( rctx->f );
    if( rv < node->header.record_size ) return 1;
    return 0;
}

struct sfbt_cached_record* sfbt_cache_node(struct sfbt_rctx* rctx, int depth) {
    struct sfbt_cached_record *rv = malloc(sizeof *rv);
    rctx->cached_bytes += sizeof *rv;
    do {
        union sfbt_record_buffer buffer;
        if( sfbt_readnode( rctx, &buffer ) ) break;

        memset( rv, 0, sizeof *rv );

        rv->data = malloc( buffer.header.record_size );
        if( !rv->data ) break;
        rctx->cached_bytes += buffer.header.record_size;

        memcpy( rv->data, buffer.raw, buffer.header.record_size );

        if( depth > 0 ) {
            int i;
            for(i=0;i<KEYS_PER_RECORD;i++) {
                const char *s = &buffer.raw[buffer.header.entry_offsets[i]];
                uint32_t *p = (uint32_t*) &s[strlen(s)+1];
                if( fseek( rctx->f, *p, SEEK_SET ) ) break;
                rv->cached[i] = sfbt_cache_node( rctx, depth - 1 );
                if( !rv->cached[i] ) break;
            }
            if( i < KEYS_PER_RECORD ) break;
        }

        return rv;
    } while(0);

    if( rv ) {
        for(int i=0;i<KEYS_PER_RECORD;i++) if( rv->cached[i] ) {
            sfbt_free_cache( rv->cached[i] );
        }
    }

    if( rv && rv->data ) free( rv->data );
    if( rv ) free(rv);

    return 0;
}

void sfbt_free_cache(struct sfbt_cached_record* cache) {
    free( cache->data );
    for(int i=0;i<KEYS_PER_RECORD;i++) if( cache->cached[i] ) {
        sfbt_free_cache( cache->cached[i] );
    }
    free( cache );
}

int sfbt_open_rctx(const char *filename, struct sfbt_rctx* rctx, int depth) {
    memset( rctx, 0, sizeof *rctx );
    rctx->f = fopen( filename, "rb" );
    if( !rctx->f ) return 1;

    do {
        rctx->root = sfbt_cache_node( rctx, depth );
        if( !rctx->root ) break;

        assert( strlen( rctx->filename ) < MAX_SFBT_FILENAME_LEN );
        strcpy( rctx->filename, filename );

        return 0;
    } while(0);
    if( rctx->root ) {
        sfbt_free_cache( rctx->root );
    }
    fclose( rctx->f );
    return 1;
}

int sfbt_close_rctx(struct sfbt_rctx* rctx) {
    if( rctx->f ) {
        fclose( rctx->f );
        rctx->f = 0;
    }
    return 0;
}

int sfbt_suspend_rctx(struct sfbt_rctx* rctx) {
    rctx->suspend = 1; // note asymmetry!

    if( rctx->f ) {
        fclose( rctx->f );
        rctx->f = 0;
    }

    return 0;
}

int sfbt_desuspend_rctx(struct sfbt_rctx* rctx) {
    if( !rctx->f ) {
        rctx->f = fopen( rctx->filename, "rb" );
        if( !rctx->f ) return 1;
    }

    return 0;
}

#ifdef TEST
int main(int argc, char *argv[]) {
    struct sfbt_record_header my;
    fprintf( stderr, "%d %d\n", sizeof my, RECORD_HEADER_SIZE );
    fprintf( stderr, "%d\n", MAX_RECORD_SIZE );
    fprintf( stderr, "%d\n", TYPICAL_RECORD_SIZE );
    fprintf( stderr, "wctx size %d\n", sizeof (struct sfbt_wctx) );

    struct sfbt_wctx *wctx = sfbt_new_wctx( "my.test.sfbt" );
    assert( wctx );
    for(int i=0;i<1000000;i++) {
        char name[100];
        sprintf( name, "%07d", i );
        if( sfbt_add_entry( wctx, name, i + 1 ) ) {
            fprintf(stderr, "failed 1.\n" );
            return 1;
        }
    }
    if( sfbt_close_wctx( wctx ) ) {
        fprintf(stderr, "failed 2.\n" );
        return 1;
    }

    struct sfbt_rctx rctx;
    if( sfbt_open_rctx( "my.test.sfbt", &rctx, 1 ) ) {
        fprintf( stderr, "readfail\n" );
        return 1;
    }
    fprintf( stderr, "Spending %d bytes on cache.\n", rctx.cached_bytes );

    fprintf( stderr, "Beginning reads.\n" );
    for(int i=0;i<1000000;i++) {
        char name[100];
        sprintf( name, "%07d", i );
        int64_t count;
        int rv = sfbt_search( &rctx, name, &count );
        if( count != i + 1 ) {
            fprintf( stderr, "oops. %d\n", i );
            return 1;
        }
    }
    fprintf( stderr, "Finished reads.\n" );

    if( sfbt_close_rctx( &rctx ) ) {
        fprintf( stderr, "readclosefail\n" );
        return 1;
    }
    
    return 0;
}
#endif
