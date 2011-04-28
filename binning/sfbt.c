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
            fprintf( stderr, "checking at %d\n", (int) ftell( wctx->f ) );
            fprintf( stderr, "wctx %p file desc %p\n", wctx, wctx->f );
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

        fprintf( stderr, "yozers %d\n", sizeof hdr );
        if( fwrite( &hdr, sizeof hdr, 1, wctx->f ) != 1 ) break;
        fprintf( stderr, "ergh\n" );

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
    fprintf( stderr, "oops %d %s\n", errno, strerror(errno) );
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
        rewind( wctx->f );
        if(sfbt_write_collected_node( wctx )) break;

        return 0;
    } while(0);
    return 1;
}

int sfbt_finalize( struct sfbt_wctx* wctx ) {
    fprintf( stderr, "o hai\n" );
    sfbt_flush_record( wctx );
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

        fprintf( stderr, "flushing record with %d entries to header pos %d\n", wctx->current_header.entries, (int) ftell( wctx->f ) );
        
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

int sfbt_add_entry( struct sfbt_wctx* wctx, const char * key, int64_t count) {
    do {
        if( wctx->current_header.entries == KEYS_PER_RECORD ) {
            if( sfbt_flush_record( wctx ) ) break;
            if( sfbt_new_leaf_record( wctx ) ) break;
        }
        assert( wctx->current_header.entries < KEYS_PER_RECORD );

        const int stringsize = strlen(key) + 1;
        static const char nulls[] = {0,0,0,0,0,0,0,0};
        fprintf( stderr, "wctx %p file desc %p\n", wctx, wctx->f );

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

        return 0;
    } while(0);
    return 1;
}

struct sfbt_wctx *sfbt_new_wctx(const char * filename) {
    struct sfbt_wctx *rv = malloc(sizeof *rv);
    do {
        memset( rv, 0, sizeof *rv );

        rv->f = fopen( filename, "wb+" );
        if( !rv->f ) break;

        if( fseek( rv->f, MAX_RECORD_SIZE, SEEK_SET ) ) break;

        rv->current_generation_foffset = ftell( rv->f );
        if( fgetpos( rv->f, &rv->current_header_pos) ) break;

        if( sfbt_new_leaf_record( rv ) ) break;

        return rv;
    } while(0);
    if( rv ) free(rv);
    return 0;
}

int sfbt_close_wctx(struct sfbt_wctx* wctx) {
    int rv = sfbt_finalize( wctx );

    fclose( wctx->f );
    free( wctx );

    return rv;
}

int main(int argc, char *argv[]) {
    struct sfbt_record_header my;
    fprintf( stderr, "%d %d\n", sizeof my, RECORD_HEADER_SIZE );
    fprintf( stderr, "%d\n", MAX_RECORD_SIZE );
    fprintf( stderr, "%d\n", TYPICAL_RECORD_SIZE );

    struct sfbt_wctx *wctx = sfbt_new_wctx( "my.test.sfbt" );
    assert( wctx );
    for(int i=0;i<500;i++) {
        char name[100];
        sprintf( name, "%d", i );
        if( sfbt_add_entry( wctx, name, i + 1 ) ) {
            fprintf(stderr, "failed 1.\n" );
            return 1;
        }
    }
    if( sfbt_close_wctx( wctx ) ) {
        fprintf(stderr, "failed 2.\n" );
        return 1;
    }
    
    return 0;
}
