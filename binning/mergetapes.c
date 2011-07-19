#include "mergetapes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int mertap_n;

void mertap_initialize(int n) {
    mertap_n = n;
}

static inline int mertap_cmp( struct mertap_record* alpha, struct mertap_record* beta) {
    for(int i=0;i<mertap_n;i++) {
        const int rv = ((int) alpha->key[i]) - ((int) beta->key[i]);
        if( rv ) return rv;
    }
    return 0;
}

void mertap_heap_read_push_or_finish( struct mertap_heap* heap, struct mertap_file* f ) {
    int rv = mertap_file_read_peek( f );
    if( !rv ) {
        mertap_heap_push( heap, f );
    } else if( rv < 0 ) {
        fprintf( stderr, "warning: read error\n" );
    }
}

struct mertap_file* mertap_heap_pop( struct mertap_heap* heap ) {
    assert( heap->no_files > 0 );
    struct mertap_file *rv = heap->files[0];
    heap->files[0] = heap->files[ (heap->no_files--) - 1 ];

    int i = 0;
    while( MERTAP_HEAP_RIGHT(i) < heap->no_files ) {
            // both children
        const int lc = MERTAP_HEAP_LEFT(i), rc = MERTAP_HEAP_RIGHT(i);
        const int to_lc = mertap_cmp( &heap->files[i]->peek, &heap->files[lc]->peek );
        const int to_rc = mertap_cmp( &heap->files[i]->peek, &heap->files[rc]->peek );
        if( to_lc < 0 && to_rc < 0 ) return rv;

        const int lc_to_rc = mertap_cmp( &heap->files[lc]->peek, &heap->files[rc]->peek );
        if( lc_to_rc < 0 ) {
            void *temp = heap->files[lc];
            heap->files[lc] = heap->files[i];
            heap->files[i] = temp;

            i = lc;
        } else {
            void *temp = heap->files[rc];
            heap->files[rc] = heap->files[i];
            heap->files[i] = temp;

            i = rc;
        }
    }

    if( MERTAP_HEAP_LEFT(i) < heap->no_files ) {
        const int lc = MERTAP_HEAP_LEFT(i), rc = MERTAP_HEAP_RIGHT(i);
        const int to_lc = mertap_cmp( &heap->files[i]->peek, &heap->files[lc]->peek );
        if( to_lc > 0 ) {
            void *temp = heap->files[lc];
            heap->files[lc] = heap->files[i];
            heap->files[i] = temp;
        }
    }

    return rv;
}

void mertap_heap_push( struct mertap_heap *heap, struct mertap_file* f ) {
    assert( heap->no_files < MAX_HEAP_FILES );
    int i = heap->no_files++;
    heap->files[ i ] = f;
    while( i ) {
        const int pi = MERTAP_HEAP_PARENT(i);
        if( mertap_cmp( &heap->files[i]->peek, &heap->files[pi]->peek ) < 0 ) {
            void *temp = heap->files[pi];
            heap->files[pi] = heap->files[i];
            heap->files[i] = temp;

            i = pi;
        } else break;
    }
}

int mertap_loop( struct mertap_file *files, int no_files, int (*f)(struct mertap_record*,void*), void* arg) {
    struct mertap_heap heap;

    heap.no_files = 0;
    for(int i=0;i<no_files;i++) {
        mertap_heap_read_push_or_finish( &heap, &files[i] );
    }

    if( MERTAP_HEAP_EMPTY( &heap ) ) return 0;

    struct mertap_record buf;

    struct mertap_file *topfile = mertap_heap_pop( &heap );
    memcpy( &buf, &topfile->peek, sizeof buf );
    mertap_heap_read_push_or_finish( &heap, topfile );

    while( !MERTAP_HEAP_EMPTY( &heap ) ) {
        topfile = mertap_heap_pop( &heap );
        if( !mertap_cmp( &buf, &topfile->peek ) ) {
            buf.count += topfile->peek.count;
        } else {
            if( f( &buf, arg ) ) return 1;
            memcpy( &buf, &topfile->peek, sizeof buf );
        }
        mertap_heap_read_push_or_finish( &heap, topfile );
    }

    if( f( &buf, arg ) ) return 1;

    return 0;
}

int mertap_file_open( struct mertap_file* f, const char *filename) {
    f->sorted_input = gzopen( filename, "rb" );
    return f->sorted_input == NULL;
}

void mertap_file_close( struct mertap_file* f ) {
    if( f->sorted_input ) {
        gzclose( f->sorted_input );
        f->sorted_input = 0;
    }
}

int mertap_file_read_peek( struct mertap_file* f ) {
    int rv;
    for(int i=0;i<mertap_n;i++) {
        rv = gzread( f->sorted_input, &f->peek.key[i], sizeof f->peek.key[i] );
        if( !rv ) return 1;
        if( rv != sizeof f->peek.key[i] ) return -1;
    }
    rv = gzread( f->sorted_input, &f->peek.count, sizeof f->peek.count );
    if( rv != sizeof f->peek.count ) return -1;
    return 0;
}
