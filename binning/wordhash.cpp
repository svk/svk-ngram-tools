#include "wordhash.hpp"

#include <string>
#include <tr1/unordered_map>

#ifdef WORDHASH_DEBUG
#include <iostream>
#endif

extern "C" {
#include "ngread.h"
}

class WordSet {
    private:
        int next;
        std::tr1::unordered_map<std::string, int> nums;

    public:
        WordSet() : next(0) {
            nums.rehash( 17961079 );
        }

        void add(const char* s) {
            nums[ s ] = next++;
        }

        int lookup(const char* s) const {
            std::tr1::unordered_map<std::string, int>::const_iterator i = nums.find( s );
            if( i != nums.end() ) {
                return i->second;
            }
            return -1;
        }

#ifdef WORDHASH_DEBUG
        void printInfo(void) const {
            std::cout << "Buckets: " << nums.bucket_count() << "/" << nums.max_bucket_count() << std::endl;
            std::cout << "Load factor: " << nums.load_factor() << "/" << nums.max_load_factor() << std::endl;
        }
#endif
};

WordHashCtx read_wordhashes(const char* filename) {
    WordSet *rv = new WordSet();
    struct ngr_file * f = ngr_open( filename );
    while( ngr_next( f ) ) {
        rv->add( ngr_s_col( f, 0 ) );
    }
    ngr_free( f );
    return (WordHashCtx) rv;
}

int lookup_wordhash( WordHashCtx ctx, const char *s ) {
    WordSet* ws = reinterpret_cast<WordSet*>( ctx );
    int rv = ws->lookup( s );
#ifdef WORDHASH_DEBUG
    std::cerr << "looking up " << s << " got result " << rv << std::endl;
#endif
    return rv;
}

void free_wordhashes(WordHashCtx ctx) {
    WordSet* ws = reinterpret_cast<WordSet*>( ctx );
    delete ws;
}
