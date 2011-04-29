#include "semifile.h"

#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

// #include <pthread.h> // TODO

struct semifile_ctx *semifile_list = 0;

struct semifile_ctx* semifile_fopen( const char* filename, int trunc ) {
    struct semifile_ctx* rv = malloc(sizeof *rv);
    do {
        const char *mode = trunc ? "wb+" : "rb+";
        FILE *f = fopen( filename, mode );
        if( !f ) break;
        fclose( f );

        assert( strlen(filename) < MAX_FILENAME_LENGTH );
        strcpy( rv->filename, filename );

        rv->fill = 0;
        rv->buffer_pos = 0;
        rv->offset = 0;

        rv->prev = 0;
        rv->next = semifile_list;
        if( rv->next ) {
            rv->next->prev = rv;
        }
        semifile_list = rv;

        rv->error = 0;
        rv->eof = 0;
        rv->errorno = 0;

        return rv;
    } while(0);
    if( rv ) free( rv );
    return 0;
}

int semifile_fclose( struct semifile_ctx* ctx ) {
    if( semifile_flush( ctx ) ) return 1;

    if( ctx->prev ) {
        ctx->prev->next = ctx->next;
    } else {
        assert( ctx == semifile_list );
        semifile_list = ctx->next;
    }

    if( ctx->next ) {
        ctx->next->prev = ctx->prev;
    }

    free( ctx );
    
    return 0;
}

int semifile_flush( struct semifile_ctx* ctx ) {
    if( !ctx->fill ) return 0;

    FILE *f = fopen( ctx->filename, "rb+" );
    if( !f ) return 1;

    if( fseek( f, ctx->buffer_pos, SEEK_SET ) ) {
        fclose( f );
        ctx->error = 2;
        return 1;
    }

    int written;
    while( written < ctx->fill ) {
        int rv = fwrite( &ctx->buffer[written], 1, ctx->fill - written, f );
        if( rv < 0 || ferror( f ) ) {
            fclose(f);
            ctx->error = 2;
            return 1;
        }
        written += rv;
    }

    ctx->buffer_pos += ctx->offset; // might not == written if seeks are involved
    ctx->fill = 0;
    ctx->offset = 0;
    memset( ctx->buffer, 0, BUFFERSIZE );

    ctx->error = ferror( f ) ? 1 : 0;
    ctx->eof = feof( f );
    ctx->errorno = errno;

    fclose(f);
    return 0;
}

long semifile_ftell( struct semifile_ctx* ctx) {
    return ctx->buffer_pos + ctx->offset;
}

int semifile_fgetpos( struct semifile_ctx* ctx, semifile_fpos_t* fpos) {
    *fpos = ctx->buffer_pos + ctx->offset;
    return 0;
}

int semifile_fsetpos( struct semifile_ctx* ctx, semifile_fpos_t* fpos ) {
    return semifile_fseek( ctx, *fpos, SEEK_SET );
}

int semifile_rewind( struct semifile_ctx* ctx ) {
    return semifile_fseek( ctx, 0, SEEK_SET );
}

int semifile_fwrite( const void* data, size_t size, size_t nmemb, struct semifile_ctx* ctx) {
    size_t total = size * nmemb;

    for(int i=0;i<2;i++) {
        if( (BUFFERSIZE - ctx->offset) >= total ) {
            memcpy( &ctx->buffer[ctx->offset], data, total );
            ctx->offset += total;
            ctx->fill = MAX( ctx->fill, ctx->offset );
            return nmemb;
        }
        semifile_flush( ctx );
    }

    FILE *f = fopen( ctx->filename, "rb+" );
    if( !f ) {
        ctx->error = 2;
        return 1;
    }

    int rv = fwrite( data, size, nmemb, f );
    if( rv > 0 ) {
        ctx->buffer_pos += size * rv;
    }

    ctx->error = ferror( f ) ? 1 : 0;
    ctx->eof = feof( f );
    ctx->errorno = errno;

    fclose(f);

    return rv;
}

int semifile_fread(void* data, size_t size, size_t nmemb, struct semifile_ctx* ctx) {
    if( semifile_flush(ctx) ) return 1;

    FILE *f = fopen( ctx->filename, "rb+" );
    if( !f ) {
        ctx->error = 2;
        return 1;
    }

    int rv = fread( data, size, nmemb, f );
    if( rv > 0 ) {
        ctx->buffer_pos += size * rv;
    }

    ctx->error = ferror( f ) ? 1 : 0;
    ctx->eof = feof( f );
    ctx->errorno = errno;

    fclose(f);

    return rv;
}

int semifile_clearerr( struct semifile_ctx* ctx ) {
    ctx->error = ctx->eof = ctx->errorno = 0;
}

int semifile_ferror( struct semifile_ctx* ctx ) {
    return ctx->error;
}

int semifile_fseek( struct semifile_ctx* ctx, long offset, int whence ) {
    switch( whence ) {
        case SEEK_SET:
            offset -= ctx->buffer_pos + ctx->offset;
            // reduce to SEEK_CUR
        case SEEK_CUR:
            {
                long noffset = offset + ctx->offset;
                if( noffset < 0 || noffset >= BUFFERSIZE ) {
                    if( semifile_flush( ctx ) ) return 1;

                    ctx->buffer_pos += offset;
                } else {
                    ctx->offset = noffset;
                }
                return 0;
            }
        case SEEK_END:
            {
                if( semifile_flush( ctx ) ) return 1;

                FILE *f = fopen( ctx->filename, "rb+" );
                if( !f ) {
                    ctx->error = 2;
                    return 1;
                }
                if( fseek( f, offset, SEEK_END ) ) {
                    fclose( f );
                    ctx->error = 2;
                    return 1;
                }
                ctx->buffer_pos = ftell( f );

                fclose(f);

                return 0;
            }
        default:
            ctx->error = 2;
            return 1;
    }
}

int semifile_feof( struct semifile_ctx* ctx ) {
    return ctx->eof;
}
