#include "tarbind.h"

int main(int argc, char *argv[]) {
    for(int i=1;i<argc;i++) {
        const char *tarfilename = argv[i];
        struct tarbind_context * ctx = tarbind_create( tarfilename );
        if( !ctx ) {
            fprintf( stderr, "error: skipping \"%s\"..\n", tarfilename );
            continue;
        }
        const char *looking_for = "tarbind.test/hello/world";
        struct tarbind_binding * binding = tarbind_get_binding( ctx, looking_for );
        char buffer[512];
        if( !binding ) {
            fprintf( stderr, "no %s in %s\n", looking_for, tarfilename );
        } else {
            lseek( ctx->descriptor, binding->offset, SEEK_SET );
            read( ctx->descriptor, buffer, sizeof buffer );
            buffer[sizeof buffer-1] = '\0';
            fprintf( stderr, "%s in %s: %s\n", looking_for, tarfilename,  buffer);
        }

        tarbind_free( ctx );
    }

    return 0;
}
