#include "unilookup.h"

#include "wordhash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashbin.h"

#include <assert.h>

static long long ipow(long long a, long long b) {
    if( !b ) return 1;
    if( b == 1 ) return a;
    long long b1 = b/2;
    long long b2 = b - b1;
    return ipow( a, b1 ) * ipow( a, b2 );
}

int unilookup_initialize_tree(struct unilookup_tree_ctx* tctx, int n, int b, int p, const char *name, const char *suffix) {
    do {
        tctx->present = 1;
        tctx->files = 0;

        tctx->n = n;
        tctx->prefix_digits = p;
        tctx->bins = b;

        tctx->no_files = ipow( b, n );

        tctx->files = malloc( sizeof *tctx->files * tctx->no_files );
        if( !tctx->files ) break;

        int xs[ n ];

        for(long long j=0;j<tctx->no_files;j++) {
            long long index = j;
            for(int k=0;k<n;k++) {
                int x = index % tctx->bins;
                index /= tctx->bins;
                xs[ n-1-k ] = x;
            }
            assert( n >= 1 );
            char filename[4096];
            strcpy( filename, name );
            if( tctx->prefix_digits > 0 ) {
                char buf[1024];
                strcpy( buf, "" );
                for(int k=0;k<tctx->prefix_digits;k++) {
                    char buf2[8];
                    sprintf( buf2, "%c", xs[k] + 'a' );
                    strcat( buf, buf2 );
                }
                strcat( filename, "/" );
                strcat( filename, buf );
            }
            strcat( filename, "/" );
            for(int k=tctx->prefix_digits;k<n;k++) {
                char buf2[8];
                sprintf( buf2, "%c", xs[k] + 'a' );
                strcat( filename, buf2 );
            }
            strcat( filename, suffix );

            int rv = sfbti_open_rctx( filename, &tctx->files[j], 0 );
            if( rv || sfbti_suspend_rctx( &tctx->files[j] ) ) {
                fprintf( stderr, "fatal error: load/suspend error on %s or out of memory\n", filename );
                // leak. but not very important xxx
                return 1;
            }
        }

        pthread_mutex_init( &tctx->mutex, 0 );

        return 0;
    } while(0);
    if( tctx->files ) free( tctx->files );
    return 1;
}

int unilookup_initialize_nbp(struct unilookup_ctx* ctx, int n, int b, int p, const char *name) {
    return unilookup_initialize_tree( &ctx->tree[ n - 1 ], n, b, p, name, ".sfbtin" );
}

int unilookup_initialize_global( struct unilookup_ctx* ctx, const char *vocab ) {
    memset( ctx, 0, sizeof *ctx );
    ctx->words = read_wordhashes( vocab );
    if( !ctx->words ) return 1;
    return 0;
}

int unilookup_initialize(struct unilookup_ctx *ctx, const char *vocab, const char *base ) {
    char buf[4096]; // overflow xx
    const int ps[] = { -1, -1, 1, 1, 2, 2 };
    const int bs[] = { -1, -1, 20, 10, 7, 5 };
    do {
        if( unilookup_initialize_global( ctx, vocab ) ) break;

        int n;
        for(n=2;n<=5;n++) {
            sprintf( buf, "%s/%dgram/", base, n );
            if( unilookup_initialize_nbp( ctx, n, bs[n], ps[n], buf ) ) break;
        }
        if( n <= 5 ) break;

        return 0;
    } while(0);
    return 1;
}

int64_t unilookup_tree_lookup_ids(struct unilookup_tree_ctx *tctx, const uint32_t *ids, int n) {
    int index = 0;
    for(int i=0;i<n;i++) {
        const int h = classify_uint32( tctx->bins, murmur_hash( (char*) &ids[i], 4 ) );
        index *= tctx->bins;
        index *= h;
    }
    int64_t count;

    pthread_mutex_lock( &tctx->mutex );

    int rv = sfbti_search( &tctx->files[index], ids, n, &count );
    
    pthread_mutex_unlock( &tctx->mutex );

    if( rv ) {
        return -1;
    }
    return count;
}

int64_t unilookup_lookup_ids(struct unilookup_ctx *ctx, const uint32_t *ids, int n) {
    if( n < 2 || n > MAX_N ) return -1;
    if( !ctx->tree[ n - 1 ].present ) return -1;

    return unilookup_tree_ctx( &ctx->tree[ n - 1 ], ids, n );
}

int64_t unilookup_lookup_words(struct unilookup_ctx *ctx, const char **tokens, int n) {
    uint32_t xs[ n ];
    for(int i=0;i<n;i++) {
        int rv = lookup_wordhash( ctx->words, tokens[i] );
        if( rv < 0 ) {
            return 0; // not an error! just a miss
        }
        xs[i] = (uint32_t) rv;
    }

    return unilookup_lookup_ids(ctx, xs, n );
}
