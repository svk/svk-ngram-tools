#ifndef H_SFBT
#define H_SFBT

#include <stdint.h>
#include <stdio.h>

#include "semifile.h"

/* Single-file B-tree.

   Note that this file is intentionally not compressed;
   compressing it would not make much sense since any
   use of involves seeking all the time.

   Note also that the generation algorithm writes to
   disk immediately and thus uses constant memory.
   This is done so several B-trees can be built in
   parallel. To achieve this the generated tree is
   slightly suboptimal.

   Note also that a detail it might be worth experimenting
   with is whether it's better to read a length field
   and then read that many bytes, or just overread
   the maximum amount from the beginning.

   Note that we assume we're getting the input
   in normal lexicographical order.

   The root node is at the zero offset. To achieve this
   we skip over the maximum amount of bytes a record
   might take, once, in the beginning.

   Neither merging nor validation is performed.
*/

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define GLOBAL_OFFSET_SIZE 4
#define MAX_KEY_SIZE 256 // includes nul
#define KEYS_PER_RECORD 128
#define KEY_ALIGNMENT 4
#define KEY_OFFSET_SIZE 2
#define COUNT_SIZE 8
#define RECORD_HEADER_SIZE (4 * 3 + KEY_OFFSET_SIZE * KEYS_PER_RECORD)
#define ENTRY_SUFFIX_SIZE MAX(COUNT_SIZE,GLOBAL_OFFSET_SIZE)
#define MAX_ENTRY_SIZE (MAX_KEY_SIZE + ENTRY_SUFFIX_SIZE)
#define MAX_RECORD_SIZE (RECORD_HEADER_SIZE + MAX_ENTRY_SIZE * KEYS_PER_RECORD)

    // guesstimates for checking viability
#define TYPICAL_ENTRY_SIZE (60 + ENTRY_SUFFIX_SIZE)
#define TYPICAL_RECORD_SIZE (RECORD_HEADER_SIZE + TYPICAL_ENTRY_SIZE * KEYS_PER_RECORD)

#define MAX_SFBT_FILENAME_LEN 1024

struct sfbt_record_header {
    uint32_t record_size; // includes this header and this field
    uint32_t entries;
    uint32_t entries_are_leaves; // convert to "flags" if necessary. 32 bit for alignment

    uint16_t entry_offsets[ KEYS_PER_RECORD ];
};

union sfbt_record_buffer {
    struct sfbt_record_header header;
    char raw[ MAX_RECORD_SIZE ];
};

struct sfbt_wctx {
    SEMIFILE* f;
    char filename[ MAX_SFBT_FILENAME_LEN ];
    long current_pos;

    long current_generation_foffset;

    struct sfbt_record_header current_header;
    semifile_fpos_t current_header_pos;
    uint16_t local_offset;

    int collected_children;
    char child_first_key[ KEYS_PER_RECORD ][ MAX_KEY_SIZE ];
    uint32_t child_offset[ KEYS_PER_RECORD ];

#ifdef DEBUG
    char debug_last_key[ MAX_ENTRY_SIZE ];
#endif
};

union sfbt_entry_suffix {
    int64_t count; // in leaf
    uint32_t offset; // in internal node
};

int sfbt_add_entry( struct sfbt_wctx*, const char *, int64_t);
int sfbt_new_leaf_record( struct sfbt_wctx*);
int sfbt_flush_record( struct sfbt_wctx*);
int sfbt_finalize( struct sfbt_wctx*);
int sfbt_write_root( struct sfbt_wctx*);
int sfbt_write_parents( struct sfbt_wctx*);
int sfbt_collect_children( struct sfbt_wctx*,long);
int sfbt_write_collected_node( struct sfbt_wctx*);
int sfbt_check_last_child_gen_at( struct sfbt_wctx*, long);

struct sfbt_wctx *sfbt_new_wctx(const char *);
int sfbt_close_wctx(struct sfbt_wctx*);

struct sfbt_cached_record {
    struct sfbt_record_header *data;
    struct sfbt_cached_record *cached[ KEYS_PER_RECORD ];
};

struct sfbt_rctx {
    FILE *f;
    char filename[ MAX_SFBT_FILENAME_LEN ];
    
    int suspend;

    struct sfbt_cached_record *root;

    union sfbt_record_buffer buffer;

    int cached_bytes;
};

struct sfbt_cached_record* sfbt_cache_node(struct sfbt_rctx*, int);
void sfbt_free_cache(struct sfbt_cached_record*);

int sfbt_find_index(union sfbt_record_buffer*, const char*,int);
int sfbt_open_rctx(const char*, struct sfbt_rctx*, int);
int sfbt_close_rctx(struct sfbt_rctx*);
int sfbt_readnode(struct sfbt_rctx*, union sfbt_record_buffer* );
int sfbt_search(struct sfbt_rctx*, const char*, int64_t*);

int sfbt_suspend_rctx(struct sfbt_rctx*);
int sfbt_desuspend_rctx(struct sfbt_rctx*);

#endif
