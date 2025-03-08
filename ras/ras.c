#include "ras.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

typedef struct _rasBlock {

    u32* code;
    u32* curr;
    size_t size; // in words

} rasBlock;

char* rasErrorStrings[] = {
    "no error",
    "failed to allocate memory",
    "register size mismatch",
    "immediate out of range",
    "invalid constant operand (shift, extend, etc)",
};

rasErrorCallback errorCallback = NULL;

static u32* ras_alloc(size_t size) {
    void* ptr =
        mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (ptr == MAP_FAILED) rasThrow(RAS_NO_MEM);
    return ptr;
}

enum Perm {
    RW,
    RX,
};

static void ras_protect(u32* code, size_t size, enum Perm perm) {
    mprotect(code, size,
             perm == RW ? PROT_READ | PROT_WRITE : PROT_READ | PROT_EXEC);
}

static void ras_free(u32* code, size_t size) {
    munmap(code, size);
}

void rasSetErrorCallback(rasErrorCallback cb) {
    errorCallback = cb;
}

rasBlock* rasCreate(size_t initialSize) {
    rasBlock* ctx = calloc(1, sizeof *ctx);

    ctx->code = ras_alloc(initialSize);
    ctx->curr = ctx->code;
    ctx->size = initialSize / 4;

    return ctx;
}

void rasDestroy(rasBlock* ctx) {
    ras_free(ctx->code, 4 * ctx->size);

    free(ctx);
}

void rasReady(rasBlock* ctx) {
    ras_protect(ctx->code, 4 * ctx->size, RX);
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
        eprintf("ras error: %s\n", rasErrorStrings[err]);
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
    ctx->code = ras_alloc(4 * ctx->size);
    ctx->curr = ctx->code;
    memcpy(ctx->code, oldCode, 4 * oldSize);
    ras_free(oldCode, 4 * oldSize);
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
