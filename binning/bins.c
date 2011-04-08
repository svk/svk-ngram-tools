#include "bins.h"

#include <stdio.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

struct bin_cons* make_bincons(long long total_count, int no_bins) {
    struct bin_cons *rv = malloc(sizeof *rv);
    if( rv ) {
        rv->remaining_count = rv->total_count = total_count;
        rv->bin_threshold = total_count / no_bins;
        rv->first_bin = rv->last_bin = 0;
        rv->target_bin_count = no_bins;
    }
    return rv;
}

int update_bincons(struct bin_cons* bc, const char *token, long long count) {
    if( !bc->last_bin
        ||
        ( (bc->bin_threshold < (bc->last_bin->count + count) )
          && bc->bin_count < bc->target_bin_count ) ) {
        struct bin_consentry *bce = malloc( sizeof *bce );
        if( bce ) {
            bce->first_token = malloc( strlen(token) + 1 );
        }
        if( !bce || !bce->first_token ) {
            if( bce ) {
                free( bce );
            }
            return 1;
        }
        strcpy( bce->first_token, token );
        bce->count = count;
        if( bc->last_bin ) {
            bc->last_bin->next_bin = bce;
        }
        if( !bc->first_bin ) {
            bc->first_bin = bce;
        }
        bc->bin_count++;
        bc->last_bin = bce;

        bc->bin_threshold = bc->remaining_count / (bc->target_bin_count - bc->bin_count + 1);
    } else {
        bc->last_bin->count += count;
    }
    bc->remaining_count -= count;
    return 0;
}

void free_bincons(struct bin_cons* bc) {
    struct bin_consentry* bce = bc->first_bin; 
    while( bce ) {
        struct bin_consentry *next = bce->next_bin;
        free( bce->first_token );
        free( bce );
        bce = next;
    }
    free( bc );
}

void free_bintable(struct bin_table *bt) {
    int i;
    if( bt->entries ) {
        for(i=0;i<bt->no_bins;i++) {
            if( bt->entries[i] ) {
                free( bt->entries[i] );
            }
        }
        free( bt->entries );
    }
    if( bt->counts ) {
        free( bt->counts );
    }
    free( bt );
}


struct bin_table* bintable_read(FILE* f) {
    struct bin_table *bt = malloc(sizeof *bt);
    do {
        if( !bt ) break;
        memset( bt, 0, sizeof *bt);

        int32_t val32;
        int64_t val64;

        if( 1 != fread( &val32, sizeof val32, 1, f ) ) break;
        bt->no_bins = val32;
        if( 1 != fread( &val64, sizeof val64, 1, f ) ) break;
        bt->total_count = val64;

        bt->entries = malloc(sizeof *bt->entries * bt->no_bins );
        if( !bt->entries ) break;
        memset( bt->entries, 0, sizeof *bt->entries * bt->no_bins );

        bt->counts = malloc(sizeof *bt->counts * bt->no_bins );
        if( !bt->counts ) break;
        memset( bt->counts, 0, sizeof *bt->counts * bt->no_bins );

        int i;
        for(i = 0;i < bt->no_bins;i++) {
            if( 1 != fread( &val32, sizeof val32, 1, f ) ) break;
            bt->entries[i] = malloc( val32 + 1 );
            if( !bt->entries[i] ) break;
            memset( bt->entries[i], 0, val32 + 1 );
            if( 1 != fread( bt->entries[i], val32, 1, f ) ) break;

            if( 1 != fread( &val64, sizeof val64, 1, f ) ) break;
            bt->counts[i] = val64;
        }
        if( i != bt->no_bins ) break;

        return bt;
    } while(0);
    if( bt ) {
        free_bintable( bt );
    }
    return 0;
}

int bincons_write(struct bin_cons *bc, FILE* f) {
    do {
        int no_bins = bc->bin_count;

        int32_t val32;
        int64_t val64;

        val32 = no_bins;
        if( 1 != fwrite( &val32, sizeof val32, 1, f ) ) break;
        val64 = bc->total_count;
        if( 1 != fwrite( &val64, sizeof val64, 1, f ) ) break;

        struct bin_consentry *bce = bc->first_bin;
        while( bce ) {
            val32 = strlen( bce->first_token );
            if( 1 != fwrite( &val32, sizeof val32, 1, f ) ) break;

            if( 1 != fwrite( bce->first_token, val32, 1, f ) ) break;

            val64 = bce->count;
            if( 1 != fwrite( &val64, sizeof val64, 1, f ) ) break;

            bce = bce->next_bin;
        }
        if( bce ) break;

        return 0;
    } while(0);
    return 1;
}

int bt_classify(struct bin_table* bt, const char* s) {
    int loops = 0;
    int mn = 0, mx = bt->no_bins - 1;
    while( mn != mx ) {
        int mp = (mn+mx+1)/2;
        assert( mn < mx );

        int cr = strcmp( s, bt->entries[mp] );

        if( cr < 0 ) {
            mx = mp - 1;
        } else if( cr >= 0 ) {
            mn = mp;
        }

        ++loops;
        assert( loops < 1000 );
    }
    return mn;
}

struct bin_table* bintable_read_fn(const char *fn) {
    FILE *f = fopen( fn, "rb" );
    if( !f ) return 0;
    struct bin_table *rv = bintable_read( f );
    fclose( f );
    return rv;
}
