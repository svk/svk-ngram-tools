#include "lib1tquery.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

int main(int argc, char *argv[]) {
    int interactive = 0;
    FILE * infile;

    if( argc < 2 ) {
        fprintf( stderr, "Usage: test-lib1tquery configfile.ini [testfile]\n" );
        return 1;
    }

    if( argc < 3 ) {
        interactive = 1;
        infile = stdin;
    } else {
        infile = fopen( argv[2], "r" );
        if( !infile ) {
            fprintf( stderr, "fatal error: unable to open test file \"%s\"\n", argv[2] );
            return 1;
        }
    }

    const char *configfile = argv[1];

    char buffer[4096], keybuf[4096];

    int failure = lib1tquery_init( configfile );
    if( failure ) {
        fprintf( stderr, "Failed to initialize, aborting.\n" );
        return 1;
    }

    struct timeval t0, t1;
    uint64_t queries = 0;

    fprintf( stderr, "Running.\n" );
    gettimeofday( &t0, 0 );

    while(1) {
        if(!fgets( buffer, 4096, infile )) break;
        char *key = strtok( buffer, "\t\n" );
        strcpy( keybuf, key );
        char *colkey = strtok( buffer, " \t" );
        int i = 0;
        int toknotfound = 0;
        int32_t iseq[5];

        while( colkey ) {
            iseq[i] = lib1tquery_dictionary( colkey );

            i++;
            colkey = strtok( 0, " \t" );
        }

        uint64_t rv = lib1tquery_lookup_ngram( i, iseq );

        ++queries;
        fprintf( stdout, "%llu\n", rv );
    }

    gettimeofday( &t1, 0 );
    fprintf( stderr, "Stopped.\n" );

    uint64_t mics0 = t0.tv_sec * 1000000L + t0.tv_usec;
    uint64_t mics1 = t1.tv_sec * 1000000L + t1.tv_usec;
    
    if( !interactive ) {
        uint64_t delta_mics = mics1 - mics0;
        double avg_mic = (double) delta_mics / (double) queries;
        fprintf( stderr, "Took %llu microseconds to process %llu queries.\n", delta_mics, queries );
        fprintf( stderr, "Mean processing time: %0.4lf microseconds.\n", avg_mic );

        fclose( infile );
    }

    fprintf( stderr, "Shutting down.\n" );
    lib1tquery_quit();
    fprintf( stderr, "Quitting.\n" );
}
