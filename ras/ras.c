#include "ras.h"
#include "ras_macros.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#define BIT(b) (1ull << (b))
#define MASK(b) (BIT(b) - 1)
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
    "invalid use of zr/sp",
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

void rasAssert(int condition, rasError err) {
    if (!condition) {
        if (errorCallback) {
            errorCallback(err);
        } else {
            fprintf(stderr, "ras error: %s\n", rasErrorStrings[err]);
            exit(1);
        }
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

int rasGenerateLogicalImm(u64 imm, u32 sf, u32* immr, u32* imms, u32* n) {
    if (!imm || !~imm) return false;
    u32 sz = sf ? 64 : 32;

    if (!sf) imm &= MASK(32);

    // find the first one bit and rotation
    u32 rot = 0;
    for (rot = 0; rot < sz; rot++) {
        if ((imm & BIT(rot)) && !(imm & BIT((rot - 1) & (sz - 1)))) {
            if (rot) imm = (imm >> rot) | (imm << (sz - rot));
            break;
        }
    }

    // find the pattern size of ones followed by zeros
    u32 ones = 0;
    for (int i = 0; i < sz; i++) {
        if (!(imm & BIT(i))) break;
        ones++;
    }
    if (ones == sz) return false;
    u32 zeros = 0;
    for (int i = ones; i < sz; i++) {
        if ((imm & BIT(i))) break;
        zeros++;
    }

    // check pattern size is power of 2
    u32 ptnsz = ones + zeros;
    u32 ptnszbits = 0;
    for (int i = 0; i < 6; i++) {
        if ((ptnsz & BIT(i))) break;
        ptnszbits++;
    }
    if (ptnsz != BIT(ptnszbits)) return false;

    // correct rotation to right rotation
    *immr = ptnsz - *immr;

    // verify pattern is correct
    for (int i = 0; i < sz >> ptnszbits; i++) {
        if ((imm & MASK(ptnsz)) != ((imm >> i * ptnsz) & MASK(ptnsz)))
            return false;
    }

    // calculate final result
    if (ptnszbits == 6) {
        *imms = ones - 1;
        *n = 1;
    } else {
        *imms = MASK(6) - MASK(ptnszbits + 1) + ones - 1;
        *n = 0;
    }
    if (rot) {
        rot &= MASK(ptnszbits);
        rot = ptnsz - rot;
    }
    *immr = rot;

    return true;
}

void rasEmitPseudoAddSubImm(rasBlock* ctx, u32 op, u32 s, rasReg rd, rasReg rn,
                            u64 imm, rasReg rtmp) {
    if (ISNBITSU(imm, 12)) {
        addsub(op, s, rd, rn, imm);
    } else if (ISNBITSU(imm, 24) && ISLOWBITS0(imm, 12)) {
        addsub(op, s, rd, rn, imm >> 12, lsl(12));
    } else {
        imm = -imm;
        if (ISNBITSU(imm, 12)) {
            addsub(!op, s, rd, rn, imm);
        } else if (ISNBITSU(imm, 24) && ISLOWBITS0(imm, 12)) {
            addsub(!op, s, rd, rn, imm >> 12, lsl(12));
        } else {
            imm = -imm;
            mov(rtmp, imm);
            addsub(op, s, rd, rn, rtmp);
        }
    }
}

void rasEmitPseudoLogicalImm(rasBlock* ctx, u32 opc, rasReg rd, rasReg rn,
                             u64 imm, rasReg rtmp) {
    u32 immr, imms, n;
    if (rasGenerateLogicalImm(imm, rd.sf, &immr, &imms, &n)) {
        logical(opc, 0, rd, rn, imm);
    } else {
        mov(rtmp, imm);
        logical(opc, 0, rd, rn, rtmp);
    }
}

void rasEmitPseudoMovImm(rasBlock* ctx, rasReg rd, u64 imm) {
    u32 immr, imms, n;
    if (rasGenerateLogicalImm(imm, rd.sf, &immr, &imms, &n)) {
        orr(rd, zr(rd.sf), imm);
        return;
    }

    int hw0s = 0;
    int hw1s = 0;

    int sz = rd.sf ? 4 : 2;

    for (int i = 0; i < sz; i++) {
        u16 hw = imm >> 16 * i;
        if (hw == 0) hw0s++;
        if (hw == MASK(16)) hw1s++;
    }

    bool neg = hw1s > hw0s;
    bool initial = true;

    for (int i = 0; i < sz; i++) {
        u16 hw = imm >> 16 * i;
        if (hw != (neg ? MASK(16) : 0)) {
            if (initial) {
                initial = false;
                if (neg) {
                    movn(rd, hw ^ MASK(16), lsl(16 * i));
                } else {
                    movz(rd, hw, lsl(16 * i));
                }
            } else {
                movk(rd, hw, lsl(16 * i));
            }
        }
    }
    if (initial) {
        if (neg) {
            movn(rd, 0);
        } else {
            movz(rd, 0);
        }
    }
}
