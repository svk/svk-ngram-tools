#include "judysort.h"

#include <Judy.h>

int main(int argc, char *argv[]) {
    judysort_initialize();

    judysort_insert_test( 10, 20, 30, 40, 50, 100 );
    judysort_insert_test( 5, 4, 3, 2, 1, 100 );
    judysort_insert_test( 5, 4, 3, 2, 1, 101 );
    judysort_insert_test( 10, 20, 30, 40, 50, 101 );
    judysort_insert_test( 1, 1, 1, 1, 1, 101 );
    judysort_insert_test( 1, 2, 3, 4, 5, 101 );

    srand( 1337 );

    for(int i=0;i<10000;i++) {
        const int k = 1000;
        int a = 1337;
        int b = rand() % k;
        int c = rand() % k;
        int d = rand() % k;
        int e = rand() % k;
        judysort_insert_test( a, b, c, d, e, 100 );
    }

    Word_t rv = judysort_dump_free( 5, judysort_dump_output_test, 0 );

    fprintf( stderr, "memory used: %ld\n", rv );

    return 0;
}
