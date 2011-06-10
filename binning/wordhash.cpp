#include "wordhash.hpp"

#include <string>
#include <tr1/unordered_map>

extern "C" {
#include "ngread.h"
}

class WordSet {
    private:
        int next;
        std::tr1::unordered_map<std::string, int> nums;

    public:
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
    return ws->lookup( s );
}

void free_wordhashes(WordHashCtx ctx) {
    WordSet* ws = reinterpret_cast<WordSet*>( ctx );
    delete ws;
}
