#include "ras.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#define MASK(b) ((1 << (b)) - 1)
#define ISNBITSU(n, b) ((u32) (n) >> (b) == 0)
#define ISNBITSS(n, b) ((s32) (n) >> (b) == 0 || (s32) (n) >> (b) == -1)
#define ISLOWBITS0(n, b) (((n) & MASK(b)) == 0)

typedef enum {
    SYM_UNDEFINED,
    SYM_INTERNAL,
    SYM_EXTERNAL,
} rasSymbolType;

typedef struct _rasSymbol {
    rasSymbolType type;
    union {
        size_t intOffset; // in words
        void* extAddr;
    };
} rasSymbol;

typedef struct _rasPatch {
    rasPatchType type;
    size_t offset; // in words
    rasLabel sym;
} rasPatch;

#define LISTNODELEN 64

#define LISTNODE(T)                                                            \
    struct ListNode_##T {                                                      \
        T d[LISTNODELEN];                                                      \
        size_t count;                                                          \
        struct ListNode_##T* next;                                             \
    }*

#define LISTNEXT(l)                                                            \
    ({                                                                         \
        if (!(l) || (l)->count == LISTNODELEN) {                               \
            typeof(l) n = malloc(sizeof *n);                                   \
            n->count = 0;                                                      \
            n->next = (l);                                                     \
            (l) = n;                                                           \
        }                                                                      \
        &(l)->d[(l)->count++];                                                 \
    })

#define LISTPOP(l)                                                             \
    ({                                                                         \
        typeof(l) tmp = (l)->next;                                             \
        free(l);                                                               \
        (l) = tmp;                                                             \
    })

typedef struct _rasBlock {

    u32* code;
    u32* curr;
    size_t size; // in words

    size_t initialSize;

    LISTNODE(rasSymbol) symbols;
    LISTNODE(rasPatch) patches;

} rasBlock;

char* rasErrorStrings[] = {
    "no error",
    "register size mismatch",
    "immediate out of range",
    "invalid constant operand (shift, extend, etc)",
    "undefined label",
    "label out of range or misaligned",
};

rasErrorCallback errorCallback = NULL;

static void* jit_alloc(size_t size) {
    // try to map near the static code
    void* ptr = mmap(rasErrorStrings, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANON, -1, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        abort();
    }
    return ptr;
}

enum Perm {
    RW,
    RX,
    RWX,
};

static void jit_protect(void* code, size_t size, enum Perm perm) {
    switch (perm) {
        case RW:
            mprotect(code, size, PROT_READ | PROT_WRITE);
            break;
        case RX:
            mprotect(code, size, PROT_READ | PROT_EXEC);
            break;
        case RWX:
            mprotect(code, size, PROT_READ | PROT_WRITE | PROT_EXEC);
            break;
    }
}

static void jit_free(void* code, size_t size) {
    munmap(code, size);
}

static void jit_clearcache(void* code, size_t size) {
    __builtin___clear_cache(code, code + size);
}

void rasSetErrorCallback(rasErrorCallback cb) {
    errorCallback = cb;
}

rasBlock* rasCreate(size_t initialSize) {
    rasBlock* ctx = calloc(1, sizeof *ctx);

    ctx->code = jit_alloc(initialSize);
    ctx->curr = ctx->code;
    ctx->size = initialSize / 4;

    ctx->initialSize = initialSize;

    ctx->symbols = NULL;
    ctx->patches = NULL;

    return ctx;
}

void rasDestroy(rasBlock* ctx) {
    jit_free(ctx->code, 4 * ctx->size);

    free(ctx);
}

rasLabel rasDeclareLabel(rasBlock* ctx) {
    rasSymbol* l = LISTNEXT(ctx->symbols);
    l->type = SYM_UNDEFINED;
    return l;
}

rasLabel rasDefineLabel(rasBlock* ctx, rasLabel l) {
    l->type = SYM_INTERNAL;
    l->intOffset = ctx->curr - ctx->code;
    return l;
}

rasLabel rasDefineLabelExternal(rasLabel l, void* addr) {
    l->type = SYM_EXTERNAL;
    l->extAddr = addr;
    return l;
}

void* rasGetLabelAddr(rasBlock* ctx, rasLabel l) {
    switch (l->type) {
        case SYM_INTERNAL:
            return ctx->code + l->intOffset;
        case SYM_EXTERNAL:
            return l->extAddr;
        case SYM_UNDEFINED:
        default:
            return NULL;
    }
}

void rasAddPatch(rasBlock* ctx, rasPatchType type, rasLabel l) {
    rasPatch* p = LISTNEXT(ctx->patches);
    p->type = type;
    p->offset = ctx->curr - ctx->code;
    p->sym = l;
}

void rasApplyPatch(rasBlock* ctx, rasPatch p) {
    void* patchaddr = &ctx->code[p.offset];
    void* symaddr = rasGetLabelAddr(ctx, p.sym);
    rasAssert(symaddr != NULL, RAS_ERR_UNDEF_LABEL);

    ptrdiff_t reladdr = symaddr - patchaddr;

    u32* patchinst = patchaddr;

    switch (p.type) {
        case RAS_PATCH_ABSADDR: {
            *(void**) patchaddr = symaddr;
            break;
        }
        case RAS_PATCH_BRANCH26: {
            rasAssert(ISLOWBITS0(reladdr, 2), RAS_ERR_BAD_LABEL);
            reladdr >>= 2;
            rasAssert(ISNBITSS(reladdr, 26), RAS_ERR_BAD_LABEL);
            reladdr &= MASK(26);
            *patchinst |= reladdr;
            break;
        }
        case RAS_PATCH_BRANCH19: {
            rasAssert(ISLOWBITS0(reladdr, 2), RAS_ERR_BAD_LABEL);
            reladdr >>= 2;
            rasAssert(ISNBITSS(reladdr, 19), RAS_ERR_BAD_LABEL);
            reladdr &= MASK(19);
            *patchinst |= reladdr << 5;
            break;
        }
    }
}

void rasApplyAllPatches(rasBlock* ctx) {
    while (ctx->patches) {
        for (int i = 0; i < ctx->patches->count; i++) {
            rasApplyPatch(ctx, ctx->patches->d[i]);
        }
        LISTPOP(ctx->patches);
    }
}

void rasReady(rasBlock* ctx) {
    rasApplyAllPatches(ctx);

    jit_protect(ctx->code, 4 * ctx->size, RX);
    jit_clearcache(ctx->code, 4 * ctx->size);
}

void rasUnready(rasBlock* ctx) {
    jit_protect(ctx->code, 4 * ctx->size, RW);
}

void* rasGetCode(rasBlock* ctx) {
    return ctx->code;
}

size_t rasGetSize(rasBlock* ctx) {
    return (ctx->curr - ctx->code) * 4;
}

void rasThrow(rasError err) {
    if (errorCallback) {
        errorCallback(err);
    } else {
        fprintf(stderr, "ras error: %s\n", rasErrorStrings[err]);
        exit(1);
    }
}

void rasAssert(int condition, rasError err) {
    if (!condition) {
        rasThrow(err);
    }
}

static void ras_grow(rasBlock* ctx) {
    u32* oldCode = ctx->code;
    size_t oldSize = ctx->size;
    ctx->size *= 2;
    ctx->code = jit_alloc(4 * ctx->size);
    ctx->curr = ctx->code;
    memcpy(ctx->code, oldCode, 4 * oldSize);
    jit_free(oldCode, 4 * oldSize);
}

void rasEmitWord(rasBlock* ctx, u32 w) {
    if (ctx->curr == ctx->code + ctx->size) ras_grow(ctx);
    *ctx->curr++ = w;
}

void rasEmitDword(rasBlock* ctx, u64 d) {
    if ((ctx->curr - ctx->code) & 1) rasEmitWord(ctx, 0);
    rasEmitWord(ctx, d);
    rasEmitWord(ctx, d >> 32);
}
