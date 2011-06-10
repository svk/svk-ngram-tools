#ifndef H_WORDHASH
#define H_WORDHASH

typedef void* WordHashCtx;

extern "C" WordHashCtx read_wordhashes(const char*);
extern "C" void free_wordhashes(WordHashCtx);

extern "C" int lookup_wordhash( WordHashCtx, const char * );

#endif
