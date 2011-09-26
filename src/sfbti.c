#include "sfbti.h"
#include <stdio.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int sfbti_check_last_child_gen_at( struct sfbti_wctx* wctx, long foffset ) {
    // are there few enough nodes in this generation that the
    // next node will be the root node?

    do {
        FSF_FPOS cpos;
        if( FSF_FGETPOS( wctx->f, &cpos ) ) break;

        if( FSF_FSEEK( wctx->f, foffset, SEEK_SET ) ) break;

        int count = 0;

        while( count <= KEYS_PER_RECORD ) {
            struct sfbti_record buffer;
            int rv = FSF_FREAD( &buffer, sizeof buffer, 1, wctx->f );
            if( rv != 1 ) {
                if( !FSF_FEOF( wctx->f ) ) return -1;
                FSF_CLEARERR( wctx->f );
                break;
            }

            count++;
        }

        // yes.
        if( FSF_FSETPOS( wctx->f, &cpos ) ) break;
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

            memcpy( &rec.keyvals[i][KEY_SIZE], &wctx->child_offset[i], OFFSET_SIZE ); 
        }
        rec.entries = wctx->collected_children;
        rec.flags = 0;

        if( FSF_FWRITE( &rec, sizeof rec, 1, wctx->f ) != 1 ) break;

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
            long foffset = FSF_FTELL( wctx->f );

            if( foffset >= stop_pos ) break;
                
            if( FSF_FREAD( &buffer, sizeof buffer, 1, wctx->f ) != 1 ) {
                return 1;
            }

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
        long next_gen_foffset = FSF_FTELL( wctx->f );

        long last_gen_cur = wctx->current_generation_foffset;
        FSF_FPOS cur;

        if( FSF_FGETPOS( wctx->f, &cur ) ) break;

        if( FSF_FSEEK( wctx->f, last_gen_cur, SEEK_SET ) ) break;
        if(sfbti_collect_children( wctx, next_gen_foffset )) break;
        last_gen_cur = FSF_FTELL( wctx->f );
        if( FSF_FSETPOS( wctx->f, &cur ) ) break;
        
        while( wctx->collected_children > 0 ) {
            if(sfbti_write_collected_node( wctx )) return 1;
            if( FSF_FGETPOS( wctx->f, &cur ) ) return 1;

            if( FSF_FSEEK( wctx->f, last_gen_cur, SEEK_SET ) ) return 1;
            if(sfbti_collect_children( wctx, next_gen_foffset )) return 1;
            last_gen_cur = FSF_FTELL( wctx->f );
            if( FSF_FSETPOS( wctx->f, &cur ) ) return 1;
        }

        wctx->current_generation_foffset = next_gen_foffset;
        return 0;
    } while(0);
    return 1;
}

int sfbti_write_root( struct sfbti_wctx* wctx ) {
    do {
        long t = FSF_FTELL( wctx->f );
        if( FSF_FSEEK( wctx->f, wctx->current_generation_foffset, SEEK_SET ) ) break;
        if(sfbti_collect_children( wctx, t )) break;
        fprintf( stderr, "[sfbti-finalize] root of tree \"%s\" has %d children\n", wctx->filename, wctx->collected_children );
        if( !wctx->collected_children ) {
            // should maybe fix this, but it is low-priority because it is not
            // necessary in the binning scheme: if the bin is empty, not
            // ... [logic fail. it is necessary. TODO]
            // [ besides, if I'm experiencing empty bins at all with the hashing
            //   scheme, that's highly likely to be a bug and so a warning is
            //   useful. ]
            fprintf( stderr, "warning: %s is empty and is left invalid\n", wctx->filename );
        } else {
            FSF_REWIND( wctx->f );
            if(sfbti_write_collected_node( wctx )) break;
        }

        return 0;
    } while(0);
    return 1;
}

int sfbti_finalize( struct sfbti_wctx* wctx ) {
    int height = 1;
    if( sfbti_flush_record( wctx ) ) return 1;
    do {
        int rv = sfbti_check_last_child_gen_at( wctx, wctx->current_generation_foffset );
        if( rv < 0 ) return 1;
        if( rv ) break;
        if( sfbti_write_parents( wctx ) ) return 1;
        height++;
    } while(1);
    height++;
    fprintf( stderr, "[sfbti-finalize] height of tree \"%s\" is %d\n", wctx->filename, height );
    if( sfbti_write_root( wctx ) ) return 1;
    return 0;
}

int sfbti_flush_record( struct sfbti_wctx* wctx ) {
    do {
//        if( FSF_FGETPOS( wctx->f, &cpos ) ) break;
//        fprintf( stderr, "wctx fpos before flush : %ld\n", cpos );

        /*

        for(int i=0;i< wctx->current_record.entries;i++) {
            int nonzero = 0;
            for(int j=0;j<KEY_SIZE;j++) {
                if( wctx->current_record.keyvals[i][j] ) nonzero = 1;
            }
            if( !nonzero ) {
                fprintf( stderr, "record has zero key: %d\n", i );
            }
        }
        */

        if( FSF_FWRITE( &wctx->current_record, sizeof wctx->current_record, 1, wctx->f ) != 1 ) break;
//        fprintf( stderr, "[beta]\n" );

//        if( FSF_FSETPOS( wctx->f, &cpos ) ) break;
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

int sfbti_add_entry( struct sfbti_wctx* wctx, const int * key, int ngram_size, int64_t count) {
    do {

        if( wctx->current_record.entries == KEYS_PER_RECORD ) {
            if( sfbti_flush_record( wctx ) ) break;
            if( sfbti_new_leaf_record( wctx ) ) break;
        }
        assert( wctx->current_record.entries < KEYS_PER_RECORD );

        int i = wctx->current_record.entries++;
        for(int j=0;j<ngram_size;j++) {
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

#ifdef USE_SEMIFILES
        rv->f = semifile_fopen( filename, 1 );
#else
        rv->f = fopen( filename, "wb+" );
#endif
        if( !rv->f ) break;
        ++phase;

        if( FSF_FSEEK( rv->f, sizeof rv->current_record, SEEK_SET ) ) break;
        ++phase;

        rv->current_generation_foffset = FSF_FTELL( rv->f );
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
        FSF_FCLOSE( wctx->f );
    }
    free( wctx );

    return rv;
}

int sfbti_find_index(struct sfbti_record* buf, const int* key, const int ngram_size, int exact) {
    int mn = 0, mx = buf->entries - 1;

    while( mn != mx ) {
        int mp = (mn + mx + 1) / 2;
        assert( mn < mx );

        int cr = 0;

        for(int i=0;i<ngram_size;i++) {
            int t = (*(int*)(&buf->keyvals[mp][TOKEN_SIZE*i]) & TOKEN_INT_MASK);
            if( key[i] < t ) {
                // arg-key is earlier than rec-key
                mx = mp - 1;
                break;
            } else if( key[i] > t || (i+1) == ngram_size ) {
                // arg-key is later or equal to rec-key
                mn = mp;
                break;
            }
        }
    }
    assert( mn == mx );

    if( exact ) {
        for(int i=0;i<ngram_size;i++) {
            int t = (*(int*)(&buf->keyvals[mn][TOKEN_SIZE*i]) & TOKEN_INT_MASK);
            if( t != key[i] ) return -1;
        }
    }
    return mn;
}

int sfbti_search(struct sfbti_rctx* rctx, const int* key, const int ngram_size, int64_t* count_out) {
    struct sfbti_cached_record *cache = rctx->root;
    struct sfbti_record *node = &cache->data;

    if( rctx->suspend ) {
        if( sfbti_desuspend_rctx( rctx ) ) return 1;
    }

    while( !(node->flags & FLAG_ENTRIES_ARE_LEAVES) ) {
        int index = sfbti_find_index( node, key, ngram_size, 0 );
#if 0
        for(int i=0;i<index;i++) {
            fprintf( stderr, "[%04d] ", i );
            for(int j=0;j<ngram_size;j++) {
                int t = (*(int*)(&node->keyvals[i][TOKEN_SIZE*j]) & TOKEN_INT_MASK);
                fprintf( stderr, "%s%d", (j>0)?":":"", t );
            }
            fprintf( stderr, "\n" );
        }
        fprintf( stderr, "finding index %d:%d:%d:%d:%d -> %d\n", key[0], key[1], key[2], key[3], key[4], index );
        for(int i=index;i<node->entries;i++) {
            fprintf( stderr, "[%04d] ", i );
            for(int j=0;j<ngram_size;j++) {
                int t = (*(int*)(&node->keyvals[i][TOKEN_SIZE*j]) & TOKEN_INT_MASK);
                fprintf( stderr, "%s%d", (j>0)?":":"", t );
            }
            fprintf( stderr, "\n" );
        }
#endif
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
    int index = sfbti_find_index( node, key, ngram_size, 1 );
    if( index < 0 ) {
//        fprintf( stderr, "unable to find %d:%d:%d:%d:%d\n", key[0], key[1], key[2], key[3], key[4] );
        *count_out = 0;
    } else {
        int64_t* countp = (int64_t*)&node->keyvals[index][KEY_SIZE];
        *count_out = *countp;
//        fprintf( stderr, "found %d:%d:%d:%d:%d -> %lld\n", key[0], key[1], key[2], key[3], key[4], *count_out );
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

#ifdef USE_OS

int sfbti_os_readnode(struct sfbti_os_rctx* rctx, struct sfbti_record* node ) {
    unsigned char *buf = (void*) node;
    ssize_t done = 0;
    while( done < sizeof *node ) {
        ssize_t rv = read( rctx->fd, &buf[done], sizeof *node - done );
        if( rv <= 0 ) {
            return 1;
        }
        done += rv;
    }
    return 0;
}

int sfbti_os_suspend_rctx(struct sfbti_os_rctx* rctx) {
    rctx->suspend = 1; // note asymmetry!

    if( rctx->fd != -1 ) {
        close( rctx->fd );
        rctx->fd = -1;
    }

    return 0;
}

int sfbti_os_desuspend_rctx(struct sfbti_os_rctx* rctx) {
    if( rctx->fd == -1 ) {
        rctx->fd = open( rctx->filename, O_RDONLY );
        if( rctx->fd == -1 ) return 1;
    }

    return 0;
}

int sfbti_os_open_rctx(const char *filename, struct sfbti_os_rctx* rctx, int depth) {
    memset( rctx, 0, sizeof *rctx );
    rctx->fd = open( filename, O_RDONLY );
    if( rctx->fd == -1 ) return 1;

    do {
        rctx->root = sfbti_os_cache_node( rctx, depth );
        if( !rctx->root ) break;

        assert( strlen( rctx->filename ) < MAX_SFBT_FILENAME_LEN );
        strcpy( rctx->filename, filename );

        return 0;
    } while(0);
    if( rctx->root ) {
        sfbti_free_cache( rctx->root );
    }
    close( rctx->fd );
    return 1;
}

int sfbti_os_close_rctx(struct sfbti_os_rctx* rctx) {
    if( rctx->fd != -1 ) {
        close( rctx->fd );
        rctx->fd = -1;
    }
    return 0;
}

struct sfbti_cached_record* sfbti_os_cache_node(struct sfbti_os_rctx* rctx, int depth) {
    struct sfbti_cached_record *rv = malloc(sizeof *rv);
    do {
        rctx->cached_bytes += sizeof *rv;
        if( !rv ) break;

        memset( rv, 0, sizeof *rv );

        if( sfbti_os_readnode( rctx, &rv->data ) ) break;

        if( depth > 0 ) {
            int i;
            for(i=0;i<KEYS_PER_RECORD;i++) {
                off_t* fpos = (off_t*) &rv->data.keyvals[i][KEY_SIZE];
                if( ((off_t)-1) == lseek( rctx->fd, *fpos, SEEK_SET ) ) break;
                rv->cached[i] = sfbti_os_cache_node( rctx, depth - 1 );
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

int sfbti_os_search(struct sfbti_os_rctx* rctx, const int* key, const int ngram_size, int64_t* count_out) {
    struct sfbti_cached_record *cache = rctx->root;
    struct sfbti_record *node = &cache->data;

    if( rctx->suspend ) {
        if( sfbti_os_desuspend_rctx( rctx ) ) return 1;
    }

    while( !(node->flags & FLAG_ENTRIES_ARE_LEAVES) ) {
        int index = sfbti_find_index( node, key, ngram_size, 0 );

        if( index < 0 ) return 1;
        if( cache && cache->cached[index] ) {
            cache = cache->cached[index];
            node = &cache->data;
        } else {
            off_t* fpos = (off_t*) &node->keyvals[index][KEY_SIZE];
            if( ((off_t)-1) == lseek( rctx->fd, *fpos, SEEK_SET ) ) return 1;
            if( sfbti_os_readnode( rctx, &rctx->buffer ) ) return 1;
            cache = 0;
            node = &rctx->buffer;
        }
    }
    int index = sfbti_find_index( node, key, ngram_size, 1 );
    if( index < 0 ) {
//        fprintf( stderr, "unable to find %d:%d:%d:%d:%d\n", key[0], key[1], key[2], key[3], key[4] );
        *count_out = 0;
    } else {
        int64_t* countp = (int64_t*)&node->keyvals[index][KEY_SIZE];
        *count_out = *countp;
//        fprintf( stderr, "found %d:%d:%d:%d:%d -> %lld\n", key[0], key[1], key[2], key[3], key[4], *count_out );
    }

    if( rctx->suspend ) {
        if( sfbti_os_suspend_rctx( rctx ) ) return 1;
    }

    return 0;
}
#endif

#ifdef USE_TAR

int sfbti_tar_readnode(struct sfbti_tar_rctx* rctx, struct sfbti_record* node ) {
    unsigned char *buf = (void*) node;
    ssize_t done = 0;
    while( done < sizeof *node ) {
        ssize_t rv = read( rctx->binding->context->descriptor, &buf[done], sizeof *node - done );
        if( rv <= 0 ) {
            return 1;
        }
        done += rv;
    }
    return 0;
}

int sfbti_tar_open_rctx(struct tarbind_context *context, const char *filename, struct sfbti_tar_rctx* rctx, int depth) {
    memset( rctx, 0, sizeof *rctx );

    fprintf( stderr, "[sfbti-tar] Getting binding for \"%s\"..", filename );
    rctx->binding = tarbind_get_binding( context, filename );
    if( !rctx->binding ) {
        fprintf( stderr, "failed!\n" );
        return 1;
    }
    tarbind_seek_to( rctx->binding );
    fprintf( stderr, "done.\n" );

    do {
        rctx->root = sfbti_tar_cache_node( rctx, depth );
        if( !rctx->root ) break;

        assert( strlen( rctx->filename ) < MAX_SFBT_FILENAME_LEN );
        strcpy( rctx->filename, filename );

        return 0;
    } while(0);
    if( rctx->root ) {
        sfbti_free_cache( rctx->root );
    }
    return 1;
}

int sfbti_tar_close_rctx(struct sfbti_tar_rctx* rctx) {
    // bindings are freed by tarbind
    return 0;
}

struct sfbti_cached_record* sfbti_tar_cache_node(struct sfbti_tar_rctx* rctx, int depth) {
    struct sfbti_cached_record *rv = malloc(sizeof *rv);
    do {
        rctx->cached_bytes += sizeof *rv;
        if( !rv ) break;

        memset( rv, 0, sizeof *rv );

        if( sfbti_tar_readnode( rctx, &rv->data ) ) break;

        if( depth > 0 ) {
            int i;
            for(i=0;i<KEYS_PER_RECORD;i++) {
                off_t* fpos = (off_t*) &rv->data.keyvals[i][KEY_SIZE];
                if( tarbind_seek_into( rctx->binding, *fpos ) ) break;
//                if( ((off_t)-1) == lseek( rctx->binding->context->descriptor, *fpos, SEEK_SET ) ) break;
                rv->cached[i] = sfbti_tar_cache_node( rctx, depth - 1 );
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

int sfbti_tar_search(struct sfbti_tar_rctx* rctx, const int* key, const int ngram_size, int64_t* count_out) {
    struct sfbti_cached_record *cache = rctx->root;
    struct sfbti_record *node = &cache->data;

    while( !(node->flags & FLAG_ENTRIES_ARE_LEAVES) ) {
        int index = sfbti_find_index( node, key, ngram_size, 0 );

        if( index < 0 ) return 1;
        if( cache && cache->cached[index] ) {
            cache = cache->cached[index];
            node = &cache->data;
        } else {
            off_t* fpos = (off_t*) &node->keyvals[index][KEY_SIZE];
//            if( ((off_t)-1) == lseek( rctx->binding->context->descriptor, *fpos, SEEK_SET ) ) return 1;
            if( tarbind_seek_into( rctx->binding, *fpos ) ) return 1;
            if( sfbti_tar_readnode( rctx, &rctx->buffer ) ) return 1;
            cache = 0;
            node = &rctx->buffer;
        }
    }
    int index = sfbti_find_index( node, key, ngram_size, 1 );
    if( index < 0 ) {
        *count_out = 0;
    } else {
        int64_t* countp = (int64_t*)&node->keyvals[index][KEY_SIZE];
        *count_out = *countp;
    }

    return 0;
}
#endif
