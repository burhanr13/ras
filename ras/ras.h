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
    RAS_ERR_BAD_R31,
    RAS_ERR_BAD_IMM,
    RAS_ERR_BAD_CONST,
    RAS_ERR_UNDEF_LABEL,
    RAS_ERR_BAD_LABEL,

    RAS_ERR_MAX
} rasError;

typedef enum {
    RAS_PATCH_ABS64,
    RAS_PATCH_REL26,
    RAS_PATCH_REL19,
    RAS_PATCH_REL21,
    RAS_PATCH_PGREL21,
    RAS_PATCH_PGOFF12,
} rasPatchType;

extern char* rasErrorStrings[];

typedef struct {
    u16 idx : 5;
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
    union {
        u32 imm;
        s32 simm;
    };
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

void rasAssert(int condition, rasError err);

rasLabel rasDeclareLabel(rasBlock* ctx);
rasLabel rasDefineLabel(rasBlock* ctx, rasLabel l);
rasLabel rasDefineLabelExternal(rasLabel l, void* addr);
void* rasGetLabelAddr(rasBlock* ctx, rasLabel l);

void rasAddPatch(rasBlock* ctx, rasPatchType type, rasLabel l);

int rasGenerateLogicalImm(u64 imm, u32 sf, u32* immr, u32* imms, u32* n);

void rasEmitWord(rasBlock* ctx, u32 w);
void rasEmitDword(rasBlock* ctx, u64 d);

#ifdef RAS_NO_CHECKS
#define rasAssert(...)
#endif

#define MASK(b) ((1 << (b)) - 1)
#define ISNBITSU(n, b) ((u32) (n) >> (b) == 0)
#define ISNBITSS(n, b)                                                         \
    ((s32) (n) >> ((b) - 1) == 0 || (s32) (n) >> ((b) - 1) == -1)
#define ISLOWBITS0(n, b) (((n) & MASK(b)) == 0)
#define CHECKR31(r, canbesp)                                                   \
    rasAssert(r.idx != 31 || r.isSp == (canbesp), RAS_ERR_BAD_R31)

#define __RAS_EMIT_DECL(name, ...)                                             \
    static inline void rasEmit##name(rasBlock* ctx, __VA_ARGS__)

__RAS_EMIT_DECL(AbsAddr, rasLabel l) {
    rasAddPatch(ctx, RAS_PATCH_ABS64, l);
    rasEmitDword(ctx, 0);
}

__RAS_EMIT_DECL(AddSubImm, u32 sf, u32 op, u32 s, rasShift shift, u32 imm12,
                rasReg rn, rasReg rd) {
    CHECKR31(rd, !s);
    CHECKR31(rn, 1);
    rasAssert(shift.type == 0, RAS_ERR_BAD_CONST);
    rasAssert(shift.amt == 0 || shift.amt == 12, RAS_ERR_BAD_CONST);
    rasAssert(ISNBITSU(imm12, 12), RAS_ERR_BAD_IMM);
    u32 sh = shift.amt == 12;
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | imm12 << 10 | sh << 22 | s << 29 |
                         op << 30 | sf << 31 | 0x11000000);
}

__RAS_EMIT_DECL(AddSubExtendedReg, u32 sf, u32 op, u32 s, rasExtend ext,
                rasReg rm, rasReg rn, rasReg rd) {
    CHECKR31(rd, !s);
    CHECKR31(rn, 1);
    CHECKR31(rm, 0);
    rasAssert(ext.amt <= 4 && !ext.invalid, RAS_ERR_BAD_CONST);
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | ext.amt << 10 | ext.type << 13 |
                         rm.idx << 16 | s << 29 | op << 30 | sf << 31 |
                         0x0b200000);
}

__RAS_EMIT_DECL(AddSubShiftedReg, u32 sf, u32 op, u32 s, rasShift shift,
                rasReg rm, rasReg rn, rasReg rd) {
    if (rd.isSp || rn.isSp) {
        rasEmitAddSubExtendedReg(ctx, sf, op, s,
                                 (rasExtend) {shift.amt, sf ? 3 : 2,
                                              shift.type != 0 || shift.amt > 4},
                                 rm, rn, rd);
        return;
    }
    CHECKR31(rm, 0);
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | shift.amt << 10 | rm.idx << 16 |
                         shift.type << 22 | s << 29 | op << 30 | sf << 31 |
                         0x0b000000);
}

__RAS_EMIT_DECL(AddSubCarry, u32 sf, u32 op, u32 s, rasReg rm, rasReg rn,
                rasReg rd) {

    CHECKR31(rd, 0);
    CHECKR31(rn, 0);
    CHECKR31(rm, 0);
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | rm.idx << 16 | s << 29 | op << 30 |
                         sf << 31 | 0x1a000000);
}

__RAS_EMIT_DECL(LogicalImm, u32 sf, u32 opc, u64 imm, rasReg rn, rasReg rd) {
    CHECKR31(rd, opc != 3);
    CHECKR31(rn, 0);
    u32 immr, imms, n;
    if (!rasGenerateLogicalImm(imm, sf, &immr, &imms, &n)) {
        rasAssert(0, RAS_ERR_BAD_IMM);
    }
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | imms << 10 | immr << 16 | n << 22 |
                         opc << 29 | sf << 31 | 0x12000000);
}

__RAS_EMIT_DECL(LogicalReg, u32 sf, u32 opc, u32 n, rasShift shift, rasReg rm,
                rasReg rn, rasReg rd) {
    CHECKR31(rd, 0);
    CHECKR31(rn, 0);
    CHECKR31(rm, 0);
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | shift.amt << 10 | rm.idx << 16 |
                         n << 21 | shift.type << 22 | opc << 29 | sf << 31 |
                         0x0a000000);
}

__RAS_EMIT_DECL(DataProc1Source, u32 sf, u32 s, u32 opcode2, u32 opcode,
                rasReg rn, rasReg rd) {
    CHECKR31(rd, 0);
    CHECKR31(rn, 0);
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | opcode << 10 | opcode2 << 16 |
                         s << 29 | sf << 31 | 0x5ac00000);
}

__RAS_EMIT_DECL(DataProc2Source, u32 sf, u32 s, rasReg rm, u32 opcode,
                rasReg rn, rasReg rd) {
    CHECKR31(rd, 0);
    CHECKR31(rn, 0);
    CHECKR31(rm, 0);
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | opcode << 10 | rm.idx << 16 |
                         s << 29 | sf << 31 | 0x1ac00000);
}

__RAS_EMIT_DECL(DataProc3Source, u32 sf, u32 op54, u32 op31, rasReg rm, u32 o0,
                rasReg ra, rasReg rn, rasReg rd) {
    CHECKR31(rd, 0);
    CHECKR31(rn, 0);
    CHECKR31(rm, 0);
    CHECKR31(ra, 0);
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | ra.idx << 10 | o0 << 15 |
                         rm.idx << 16 | op31 << 21 | op54 << 29 | sf << 31 |
                         0x1b000000);
}

__RAS_EMIT_DECL(CondSelect, u32 sf, u32 op, u32 s, rasReg rm, u32 cond, u32 op2,
                rasReg rn, rasReg rd) {

    CHECKR31(rd, 0);
    CHECKR31(rn, 0);
    CHECKR31(rm, 0);
    rasEmitWord(ctx, rd.idx | rn.idx << 5 | op2 << 10 | cond << 12 |
                         rm.idx << 16 | s << 29 | op << 30 | sf << 31 |
                         0x1a800000);
}

__RAS_EMIT_DECL(PCRelAddr, u32 op, rasLabel lab, rasReg rd) {
    CHECKR31(rd, 0);
    rasAddPatch(ctx, op ? RAS_PATCH_PGREL21 : RAS_PATCH_REL21, lab);
    rasEmitWord(ctx, rd.idx | op << 31 | 0x10000000);
}

__RAS_EMIT_DECL(MoveWide, u32 sf, u32 opc, rasShift shift, u32 imm16,
                rasReg rd) {
    CHECKR31(rd, 0);
    rasAssert(ISNBITSU(imm16, 16), RAS_ERR_BAD_IMM);
    rasAssert(shift.type == 0, RAS_ERR_BAD_CONST);
    if (sf) {
        rasAssert(shift.amt == 0 || shift.amt == 16 || shift.amt == 32 ||
                      shift.amt == 48,
                  RAS_ERR_BAD_CONST);
    } else {
        rasAssert(shift.amt == 0 || shift.amt == 16, RAS_ERR_BAD_CONST);
    }
    u32 hw = shift.amt / 16;
    rasEmitWord(ctx, rd.idx | imm16 << 5 | hw << 21 | opc << 29 | sf << 31 |
                         0x12800000);
}

__RAS_EMIT_DECL(LoadStoreImmOff, u32 size, u32 opc, rasAddrImm amod,
                rasReg rt) {
    CHECKR31(rt, 0);
    CHECKR31(amod.rn, 1);
    if (amod.mode == 0 && ISLOWBITS0(amod.imm, size) &&
        ISNBITSU(amod.imm >> size, 12)) {
        amod.imm >>= size;
        rasEmitWord(ctx, rt.idx | amod.rn.idx << 5 | amod.imm << 10 |
                             opc << 22 | size << 30 | 0x39000000);
    } else {
        rasAssert(ISNBITSS(amod.imm, 9), RAS_ERR_BAD_IMM);
        amod.imm &= MASK(9);
        rasEmitWord(ctx, rt.idx | amod.rn.idx << 5 | amod.mode << 10 |
                             amod.imm << 12 | opc << 22 | size << 30 |
                             0x38000000);
    }
}

__RAS_EMIT_DECL(LoadStoreRegOff, u32 size, u32 opc, rasAddrReg amod,
                rasReg rt) {
    CHECKR31(rt, 0);
    CHECKR31(amod.rn, 1);
    CHECKR31(amod.rm, 0);
    rasAssert(!amod.ext.invalid, RAS_ERR_BAD_CONST);
    rasAssert(amod.ext.type & 2, RAS_ERR_BAD_CONST);
    rasAssert(amod.ext.amt == 0 || amod.ext.amt == size, RAS_ERR_BAD_CONST);
    u32 s = amod.ext.amt != 0;
    rasEmitWord(ctx, rt.idx | amod.rn.idx << 5 | s << 12 | amod.ext.type << 13 |
                         amod.rm.idx << 16 | opc << 22 | size << 30 |
                         0x38200800);
}

__RAS_EMIT_DECL(LoadLiteral, u32 opc, rasLabel l, rasReg rt) {
    CHECKR31(rt, 0);
    rasAddPatch(ctx, RAS_PATCH_REL19, l);
    rasEmitWord(ctx, rt.idx | opc << 30 | 0x18000000);
}

__RAS_EMIT_DECL(LoadStorePair, u32 opc, u32 l, rasAddrImm amod, rasReg rt2,
                rasReg rt) {
    CHECKR31(rt, 0);
    CHECKR31(rt2, 0);
    CHECKR31(amod.rn, 1);
    if (amod.mode == 0) amod.mode = 2;
    u32 size = (opc & 2) ? 3 : 2;
    rasAssert(ISLOWBITS0(amod.imm, size), RAS_ERR_BAD_IMM);
    amod.simm >>= size;
    rasAssert(ISNBITSS(amod.imm, 7), RAS_ERR_BAD_IMM);
    amod.imm &= MASK(7);
    rasEmitWord(ctx, rt.idx | amod.rn.idx << 5 | rt2.idx << 10 |
                         amod.imm << 15 | l << 22 | amod.mode << 23 |
                         opc << 30 | 0x28000000);
}

__RAS_EMIT_DECL(BranchUncondImm, u32 op, rasLabel lab) {
    rasAddPatch(ctx, RAS_PATCH_REL26, lab);
    rasEmitWord(ctx, op << 31 | 0x14000000);
}

__RAS_EMIT_DECL(BranchCondImm, rasLabel lab, u32 o0, u32 cond) {
    rasAddPatch(ctx, RAS_PATCH_REL19, lab);
    rasEmitWord(ctx, cond | o0 << 4 | 0x54000000);
}

__RAS_EMIT_DECL(BranchReg, u32 opc, u32 op2, u32 op3, rasReg rn, u32 op4) {
    CHECKR31(rn, 0);
    rasEmitWord(ctx, op4 | rn.idx << 5 | op3 << 10 | op2 << 16 | opc << 21 |
                         0xd6000000);
}

__RAS_EMIT_DECL(Hint, u32 crm, u32 op2) {
    rasEmitWord(ctx, op2 << 5 | crm << 8 | 0xd503201f);
}

#undef MASK
#undef ISNBITSU
#undef ISNBITSS

void rasEmitPseudoAddSubImm(rasBlock* ctx, u32 sf, u32 op, u32 s, rasReg rd,
                            rasReg rn, u64 imm, rasReg rtmp);
void rasEmitPseudoLogicalImm(rasBlock* ctx, u32 sf, u32 opc, rasReg rd,
                             rasReg rn, u64 imm, rasReg rtmp);
void rasEmitPseudoMovImm(rasBlock* ctx, u32 sf, rasReg rd, u64 imm);
void rasEmitPseudoMovReg(rasBlock* ctx, u32 sf, rasReg rd, rasReg rm);
void rasEmitPseudoPCRelAddrLong(rasBlock* ctx, rasReg rd, rasLabel lab);

#endif
