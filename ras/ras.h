#ifndef __RAS_H
#define __RAS_H

#include <stddef.h>
#include <stdint.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;

typedef struct _rasBlock rasBlock;

typedef struct _rasSymbol* rasLabel;

typedef enum {
    RAS_OK,
    RAS_ERR_BAD_REG_SIZE,
    RAS_ERR_BAD_IMM,
    RAS_ERR_BAD_CONST,
    RAS_ERR_UNDEF_LABEL,
    RAS_ERR_BAD_LABEL,
} rasError;

typedef enum {
    RAS_PATCH_ABSADDR,
    RAS_PATCH_BRANCH26,
    RAS_PATCH_BRANCH19,
} rasPatchType;

extern char* rasErrorStrings[];

typedef struct {
    u16 idx : 5;
    u16 sf : 1;
    u16 isSp : 1;
} rasReg;

typedef struct {
    u8 amt : 6;
    u8 type : 2;
} rasShift;

typedef struct {
    u8 amt : 3;
    u8 type : 3;
    u8 invalid : 1;
} rasExtend;

typedef struct {
    rasReg rn;
    u16 mode : 2;
    u32 imm;
} rasAddrImm;

typedef struct {
    rasReg rn;
    rasReg rm;
    rasExtend ext;
} rasAddrReg;

typedef void (*rasErrorCallback)(rasError);

void rasSetErrorCallback(rasErrorCallback cb);

rasBlock* rasCreate(size_t initialSize);
void rasDestroy(rasBlock* ctx);

void rasReady(rasBlock* ctx);
void rasUnready(rasBlock* ctx);
void* rasGetCode(rasBlock* ctx);
size_t rasGetSize(rasBlock* ctx);

void rasThrow(rasError err);
void rasAssert(int condition, rasError err);

rasLabel rasDeclareLabel(rasBlock* ctx);
rasLabel rasDefineLabel(rasBlock* ctx, rasLabel l);
rasLabel rasDefineLabelExternal(rasLabel l, void* addr);
void* rasGetLabelAddr(rasBlock* ctx, rasLabel l);

void rasAddPatch(rasBlock* ctx, rasPatchType type, rasLabel l);

void rasEmitWord(rasBlock* ctx, u32 w);
void rasEmitDword(rasBlock* ctx, u64 d);

#define MASK(b) ((1 << (b)) - 1)
#define ISNBITSU(n, b) ((u32) (n) >> (b) == 0)
#define ISNBITSS(n, b) ((s32) (n) >> (b) == 0 || (s32) (n) >> (b) == -1)
#define ISLOWBITS0(n, b) (((n) & MASK(b)) == 0)

#define __RAS_EMIT_DECL(name, ...)                                             \
    static inline void rasEmit##name(rasBlock* ctx, __VA_ARGS__)

__RAS_EMIT_DECL(AbsAddr, rasLabel l) {
    rasAddPatch(ctx, RAS_PATCH_ABSADDR, l);
    rasEmitDword(ctx, 0);
}

__RAS_EMIT_DECL(AddSubImm, u32 op, u32 s, rasShift shift, u32 imm12, rasReg rn,
                rasReg rd) {
    rasAssert(rn.sf == rd.sf, RAS_ERR_BAD_REG_SIZE);
    rasAssert(shift.type == 0, RAS_ERR_BAD_CONST);
    rasAssert(shift.amt == 0 || shift.amt == 12, RAS_ERR_BAD_CONST);
    rasAssert(ISNBITSU(imm12, 12), RAS_ERR_BAD_IMM);
    u32 sh = shift.amt == 12;
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | imm12 << 10 | sh << 22 | s << 29 |
                         op << 30 | rn.sf << 31 | 0x11000000);
}

__RAS_EMIT_DECL(AddSubShiftedReg, u32 op, u32 s, rasShift shift, rasReg rm,
                rasReg rn, rasReg rd) {
    rasAssert(rd.sf == rm.sf && rd.sf == rn.sf, RAS_ERR_BAD_REG_SIZE);
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | shift.amt << 10 | rm.idx << 16 |
                         shift.type << 22 | s << 29 | op << 30 | rd.sf << 31 |
                         0x0b000000);
}

__RAS_EMIT_DECL(AddSubExtendedReg, u32 op, u32 s, rasExtend ext, rasReg rm,
                rasReg rn, rasReg rd) {
    rasAssert(rd.sf == rn.sf, RAS_ERR_BAD_REG_SIZE);
    if (!rd.sf) rasAssert(!rm.sf, RAS_ERR_BAD_REG_SIZE);
    if (!rm.sf) rasAssert((ext.type & 3) != 3, RAS_ERR_BAD_REG_SIZE);
    rasAssert(ext.amt <= 4, RAS_ERR_BAD_CONST);
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | ext.amt << 10 | ext.type << 13 |
                         rm.idx << 16 | s << 29 | op << 30 | rd.sf << 31 |
                         0x0b200000);
}

__RAS_EMIT_DECL(MoveWide, u32 opc, rasShift shift, u32 imm16, rasReg rd) {
    rasAssert(ISNBITSU(imm16, 16), RAS_ERR_BAD_IMM);
    rasAssert(shift.type == 0, RAS_ERR_BAD_CONST);
    if (rd.sf) {
        rasAssert(shift.amt == 0 || shift.amt == 16 || shift.amt == 32 ||
                      shift.amt == 48,
                  RAS_ERR_BAD_CONST);
    } else {
        rasAssert(shift.amt == 0 || shift.amt == 16, RAS_ERR_BAD_CONST);
    }
    u32 hw = shift.amt / 16;
    rasEmitWord(ctx, rd.idx | imm16 << 5 | hw << 21 | opc << 29 | rd.sf << 31 |
                         0x12800000);
}

// inputs are bit simpler than actual encoding here
// size: 0=byte 1=half 2=from rt
// opc: 0=store 1=load unsigned 2=load signed
__RAS_EMIT_DECL(LoadStoreImmOff, u32 size, u32 opc, rasAddrImm amod,
                rasReg rt) {
    rasAssert(amod.rn.sf, RAS_ERR_BAD_REG_SIZE);
    if (rt.sf) {
        if (size == 2 && opc != 2) size = 3;
    } else {
        rasAssert(!(size == 2 && opc == 2), RAS_ERR_BAD_REG_SIZE);
        if (opc == 2) opc = 3;
    }
    if (ISNBITSS(amod.imm, 9)) {
        amod.imm &= MASK(9);
        rasEmitWord(ctx, rt.idx | amod.rn.idx << 5 | amod.mode << 10 |
                             amod.imm << 12 | opc << 22 | size << 30 |
                             0x38000000);
    } else {
        // pre/post index can only be used with simm9
        rasAssert(amod.mode == 0, RAS_ERR_BAD_IMM);
        rasAssert(ISLOWBITS0(amod.imm, size), RAS_ERR_BAD_IMM);
        amod.imm >>= size;
        rasAssert(ISNBITSU(amod.imm, 12), RAS_ERR_BAD_IMM);
        rasEmitWord(ctx, rt.idx | amod.rn.idx << 5 | amod.imm << 10 |
                             opc << 22 | size << 30 | 0x39000000);
    }
}

__RAS_EMIT_DECL(LoadStoreRegOff, u32 size, u32 opc, rasAddrReg amod,
                rasReg rt) {
    rasAssert(!amod.ext.invalid, RAS_ERR_BAD_CONST);
    rasAssert(amod.rn.sf, RAS_ERR_BAD_REG_SIZE);
    if (rt.sf) {
        if (size == 2 && opc != 2) size = 3;
    } else {
        rasAssert(!(size == 2 && opc == 2), RAS_ERR_BAD_REG_SIZE);
        if (opc == 2) opc = 3;
    }
    rasAssert(amod.ext.type & 2, RAS_ERR_BAD_CONST);
    if (!amod.rm.sf) rasAssert(!(amod.ext.type & 1), RAS_ERR_BAD_REG_SIZE);
    rasAssert(amod.ext.amt == 0 || amod.ext.amt == size, RAS_ERR_BAD_CONST);
    u32 s = amod.ext.amt != 0;
    rasEmitWord(ctx, rt.idx | amod.rn.idx << 5 | s << 12 | amod.ext.type << 13 |
                         amod.rm.idx << 16 | opc << 22 | size << 30 |
                         0x38200800);
}

__RAS_EMIT_DECL(BranchUncondImm, u32 op, rasLabel lab) {
    rasAddPatch(ctx, RAS_PATCH_BRANCH26, lab);
    rasEmitWord(ctx, op << 31 | 0x14000000);
}

__RAS_EMIT_DECL(BranchCondImm, rasLabel lab, u32 o0, u32 cond) {
    rasAddPatch(ctx, RAS_PATCH_BRANCH19, lab);
    rasEmitWord(ctx, cond | o0 << 4 | 0x54000000);
}

__RAS_EMIT_DECL(BranchReg, u32 opc, u32 op2, u32 op3, rasReg rn,
                u32 op4) {
    rasAssert(rn.sf, RAS_ERR_BAD_REG_SIZE);
    rasEmitWord(ctx, op4 | rn.idx << 5 | op3 << 10 | op2 << 16 | opc << 21 |
                         0xd6000000);
}

__RAS_EMIT_DECL(Hint, u32 crm, u32 op2) {
    rasEmitWord(ctx, op2 << 5 | crm << 8 | 0xd503201f);
}

#undef MASK
#undef ISNBITSU
#undef ISNBITSS

#endif
