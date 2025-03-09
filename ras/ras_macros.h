#ifndef __RAS_MACROS_H
#define __RAS_MACROS_H

#include "ras.h"

#ifndef RAS_CTX_VAR
#define RAS_CTX_VAR ctx
#endif

#define __EMIT(name, ...) rasEmit##name(RAS_CTX_VAR, __VA_ARGS__)

#define __ID(...) __VA_ARGS__
#define __VA_IF_1(t, f, ...) t
#define __VA_IF_(t, f, ...) f
#define __VA_IF(t, f, ...) __VA_IF_##__VA_OPT__(1)(__ID(t), f, __VA_ARGS__)
#define __VA_DFL(dfl, ...) __VA_IF(__ID(__VA_ARGS__), dfl, __VA_ARGS__)

// unfortunately generic requires all branches to be well typed
// solve this by using more generic
// this solution using an undefined symbol is from
// https://www.chiark.greenend.org.uk/~sgtatham/quasiblog/c11-generic/#coercion
extern void* _ras_invalid_argument_type;
#define __OP_IMM(op)                                                           \
    _Generic(op,                                                               \
        rasReg: *(int*) _ras_invalid_argument_type,                            \
        rasLabel: *(int*) _ras_invalid_argument_type,                          \
        default: op)
#define __FORCE(type, val)                                                     \
    _Generic(val, type: val, default: *(type*) _ras_invalid_argument_type)

#define __CINV(n, v) ((n) ? ~(v) : (v))

#define word(w) __EMIT(Word, w)
#define dword(d)                                                               \
    _Generic(d,                                                                \
        rasLabel: __EMIT(AbsAddr, __FORCE(rasLabel, d)),                       \
        default: __EMIT(Dword, __OP_IMM(d)))

#define addsub(sf, op, s, rd, rn, op2, ...)                                    \
    _addsub(sf, op, s, rd, rn, op2, __VA_DFL(lsl(0), __VA_ARGS__))
#define _addsub(sf, op, s, rd, rn, op2, mod)                                   \
    _Generic(op2,                                                              \
        rasReg: _Generic(mod,                                                  \
            rasShift: __EMIT(AddSubShiftedReg, sf, op, s,                      \
                             __FORCE(rasShift, mod), __FORCE(rasReg, op2), rn, \
                             rd),                                              \
            default: __EMIT(AddSubExtendedReg, sf, op, s,                      \
                            __FORCE(rasExtend, mod), __FORCE(rasReg, op2), rn, \
                            rd)),                                              \
        default: _Generic(mod,                                                 \
            rasReg: __EMIT(PseudoAddSubImm, sf, op, s, rd, rn, __OP_IMM(op2),  \
                           __FORCE(rasReg, mod)),                              \
            default: __EMIT(AddSubImm, sf, op, s, __FORCE(rasShift, mod),      \
                            __OP_IMM(op2), rn, rd)))

#define add(rd, rn, op2, ...) addsub(0, 0, 0, rd, rn, op2, __VA_ARGS__)
#define adds(rd, rn, op2, ...) addsub(0, 0, 1, rd, rn, op2, __VA_ARGS__)
#define sub(rd, rn, op2, ...) addsub(0, 1, 0, rd, rn, op2, __VA_ARGS__)
#define subs(rd, rn, op2, ...) addsub(0, 1, 1, rd, rn, op2, __VA_ARGS__)
#define addx(rd, rn, op2, ...) addsub(1, 0, 0, rd, rn, op2, __VA_ARGS__)
#define addsx(rd, rn, op2, ...) addsub(1, 0, 1, rd, rn, op2, __VA_ARGS__)
#define subx(rd, rn, op2, ...) addsub(1, 1, 0, rd, rn, op2, __VA_ARGS__)
#define subsx(rd, rn, op2, ...) addsub(1, 1, 1, rd, rn, op2, __VA_ARGS__)
#define cmp(rn, op2, ...) subs(zr, rn, op2, __VA_ARGS__)
#define cmn(rn, op2, ...) adds(zr, rn, op2, __VA_ARGS__)
#define cmpx(rn, op2, ...) subsx(zr, rn, op2, __VA_ARGS__)
#define cmnx(rn, op2, ...) addsx(zr, rn, op2, __VA_ARGS__)

#define logical(sf, opc, n, rd, rn, op2, ...)                                  \
    _logical(sf, opc, n, rd, rn, op2, __VA_DFL(lsl(0), __VA_ARGS__))
#define _logical(sf, opc, n, rd, rn, op2, mod)                                 \
    _Generic(op2,                                                              \
        rasReg: __EMIT(LogicalReg, sf, opc, n, __FORCE(rasShift, mod),         \
                       __FORCE(rasReg, op2), rn, rd),                          \
        default: _Generic(mod,                                                 \
            rasReg: __EMIT(PseudoLogicalImm, sf, opc, rd, rn,                  \
                           __CINV(n, __OP_IMM(op2)), __FORCE(rasReg, mod)),    \
            default: __EMIT(LogicalImm, sf, opc, __CINV(n, __OP_IMM(op2)), rn, \
                            rd)))

#define and(rd, rn, op2, ...) logical(0, 0, 0, rd, rn, op2, __VA_ARGS__)
#define bic(rd, rn, op2, ...) logical(0, 0, 1, rd, rn, op2, __VA_ARGS__)
#define orr(rd, rn, op2, ...) logical(0, 1, 0, rd, rn, op2, __VA_ARGS__)
#define orn(rd, rn, op2, ...) logical(0, 1, 1, rd, rn, op2, __VA_ARGS__)
#define eor(rd, rn, op2, ...) logical(0, 2, 0, rd, rn, op2, __VA_ARGS__)
#define eon(rd, rn, op2, ...) logical(0, 2, 1, rd, rn, op2, __VA_ARGS__)
#define ands(rd, rn, op2, ...) logical(0, 3, 0, rd, rn, op2, __VA_ARGS__)
#define bics(rd, rn, op2, ...) logical(0, 3, 1, rd, rn, op2, __VA_ARGS__)
#define andx(rd, rn, op2, ...) logical(1, 0, 0, rd, rn, op2, __VA_ARGS__)
#define bicx(rd, rn, op2, ...) logical(1, 0, 1, rd, rn, op2, __VA_ARGS__)
#define orrx(rd, rn, op2, ...) logical(1, 1, 0, rd, rn, op2, __VA_ARGS__)
#define ornx(rd, rn, op2, ...) logical(1, 1, 1, rd, rn, op2, __VA_ARGS__)
#define eorx(rd, rn, op2, ...) logical(1, 2, 0, rd, rn, op2, __VA_ARGS__)
#define eonx(rd, rn, op2, ...) logical(1, 2, 1, rd, rn, op2, __VA_ARGS__)
#define andsx(rd, rn, op2, ...) logical(1, 3, 0, rd, rn, op2, __VA_ARGS__)
#define bicsx(rd, rn, op2, ...) logical(1, 3, 1, rd, rn, op2, __VA_ARGS__)
#define mvn(rn, rm) orn(rn, zr, rm)
#define tst(rn, op2, ...) ands(zr, rn, op2, __VA_ARGS__)
#define mvnx(rn, rm) ornx(rn, zr, rm)
#define tstx(rn, op2, ...) andsx(zr, rn, op2, __VA_ARGS__)

#define lsl(s, ...) ((rasShift) {s, 0})
#define lsr(s, ...) ((rasShift) {s, 1})
#define asr(s, ...) ((rasShift) {s, 2})

#define extend(type, ...) _extend(type, __VA_DFL(0, __VA_ARGS__))
#define _extend(type, s, ...) ((rasExtend) {s, type})

#define uxtb(...) extend(0, __VA_ARGS__)
#define uxth(...) extend(1, __VA_ARGS__)
#define uxtw(...) extend(2, __VA_ARGS__)
#define uxtx(...) extend(3, __VA_ARGS__)
#define sxtb(...) extend(4, __VA_ARGS__)
#define sxth(...) extend(5, __VA_ARGS__)
#define sxtw(...) extend(6, __VA_ARGS__)
#define sxtx(...) extend(7, __VA_ARGS__)

#define movewide(sf, opc, rd, imm, ...)                                        \
    __EMIT(MoveWide, sf, opc, __VA_DFL(lsl(0), __VA_ARGS__), imm, rd)

#define movn(rd, imm, ...) movewide(0, 0, rd, imm, __VA_ARGS__)
#define movz(rd, imm, ...) movewide(0, 2, rd, imm, __VA_ARGS__)
#define movk(rd, imm, ...) movewide(0, 3, rd, imm, __VA_ARGS__)
#define movnx(rd, imm, ...) movewide(1, 0, rd, imm, __VA_ARGS__)
#define movzx(rd, imm, ...) movewide(1, 2, rd, imm, __VA_ARGS__)
#define movkx(rd, imm, ...) movewide(1, 3, rd, imm, __VA_ARGS__)

#define mov(rd, op2)                                                           \
    _Generic(op2,                                                              \
        rasReg: _movreg(rd, __FORCE(rasReg, op2)),                             \
        default: __EMIT(PseudoMovImm, 0, rd, __OP_IMM(op2)))
#define _movreg(rd, rm)                                                        \
    ((rd).isSp || (rm).isSp ? add(rd, rm, 0) : orr(rd, zr, rm))
#define movx(rd, op2)                                                          \
    _Generic(op2,                                                              \
        rasReg: _movregx(rd, __FORCE(rasReg, op2)),                             \
        default: __EMIT(PseudoMovImm, 1, rd, __OP_IMM(op2)))
#define _movregx(rd, rm)                                                       \
    ((rd).isSp || (rm).isSp ? addx(rd, rm, 0) : orrx(rd, zr, rm))

#define __EXT_OF_SHIFT(s)                                                      \
    _Generic(s,                                                                \
        rasShift: ((rasExtend) {s.amt, 3, s.type != 0 || s.amt > 4}),          \
        rasExtend: s)

#define ptr(rn, ...) _ptr(rn, __VA_DFL(0, __VA_ARGS__))
#define _ptr(x, y) __ptr(x, y)
#define __ptr(rn, off, ...)                                                    \
    _Generic(off,                                                              \
        rasReg: ((rasAddrReg) {                                                \
            rn, __FORCE(rasReg, off),                                          \
            __EXT_OF_SHIFT(__VA_DFL(lsl(0), __VA_ARGS__))}),                   \
        default: ((rasAddrImm) {rn, 0, {__OP_IMM(off)}}))

#define post_ptr(rn, off) ((rasAddrImm) {rn, 1, {off}})
#define pre_ptr(rn, off) ((rasAddrImm) {rn, 3, {off}})

#define loadstore(size, opc, rt, amod)                                         \
    _Generic(amod,                                                             \
        rasAddrReg: __EMIT(LoadStoreRegOff, size, opc,                         \
                           __FORCE(rasAddrReg, amod), rt),                     \
        rasAddrImm: __EMIT(LoadStoreImmOff, size, opc,                         \
                           __FORCE(rasAddrImm, amod), rt),                     \
        rasLabel: (void) 0)

#define strb(rt, amod) loadstore(0, 0, rt, amod)
#define ldrb(rt, amod) loadstore(0, 1, rt, amod)
#define ldrsbx(rt, amod) loadstore(0, 2, rt, amod)
#define ldrsb(rt, amod) loadstore(0, 3, rt, amod)
#define strh(rt, amod) loadstore(1, 0, rt, amod)
#define ldrh(rt, amod) loadstore(1, 1, rt, amod)
#define ldrshx(rt, amod) loadstore(1, 2, rt, amod)
#define ldrsh(rt, amod) loadstore(1, 3, rt, amod)
#define str(rt, amod) loadstore(2, 0, rt, amod)
#define _ldr(rt, amod) loadstore(2, 1, rt, amod)
#define _ldrswx(rt, amod) loadstore(2, 2, rt, amod)
#define strx(rt, amod) loadstore(3, 0, rt, amod)
#define _ldrx(rt, amod) loadstore(3, 1, rt, amod)

#define loadliteral(opc, rt, l) __EMIT(LoadLiteral, opc, l, rt)

#define ldr(rt, amod)                                                          \
    _Generic(amod,                                                             \
        rasLabel: loadliteral(0, rt, __FORCE(rasLabel, amod)),                 \
        default: _ldr(rt, amod))
#define ldrx(rt, amod)                                                         \
    _Generic(amod,                                                             \
        rasLabel: loadliteral(1, rt, __FORCE(rasLabel, amod)),                 \
        default: _ldrx(rt, amod))
#define ldrswx(rt, amod)                                                        \
    _Generic(amod,                                                             \
        rasLabel: loadliteral(2, rt, __FORCE(rasLabel, amod)),                 \
        default: _ldrswx(rt, amod))

#define loadstorepair(opc, l, rt, rt2, amod)                                   \
    __EMIT(LoadStorePair, opc, l, amod, rt2, rt)

#define stp(rt, rt2, amod) loadstorepair(0, 0, rt, rt2, amod)
#define ldp(rt, rt2, amod) loadstorepair(0, 1, rt, rt2, amod)
#define ldpswx(rt, rt2, amod) loadstorepair(1, 1, rt, rt2, amod)
#define stpx(rt, rt2, amod) loadstorepair(2, 0, rt, rt2, amod)
#define ldpx(rt, rt2, amod) loadstorepair(2, 1, rt, rt2, amod)
#define push(rt, rt2) stpx(rt, rt2, pre_ptr(sp, -0x10))
#define pop(rt, rt2) ldpx(rt, rt2, post_ptr(sp, 0x10))

#define branchuncondimm(op, l) __EMIT(BranchUncondImm, op, l)
#define branchcondimm(c, o0, l) __EMIT(BranchCondImm, l, o0, c)

#define b(l, ...)                                                              \
    __VA_IF(branchcondimm(l, 0, __VA_ARGS__), branchuncondimm(0, l),           \
            __VA_ARGS__)
#define bl(l) branchuncondimm(1, l)

#define hint(crm, op2) __EMIT(Hint, crm, op2)

#define nop() hint(0, 0)

#define branchreg(opc, op2, op3, op4, rn)                                      \
    __EMIT(BranchReg, opc, op2, op3, rn, op4)

#define br(rn) branchreg(0, 31, 0, 0, rn)
#define blr(rn) branchreg(1, 31, 0, 0, rn)
#define ret(...) _ret(__VA_DFL(lr, __VA_ARGS__))
#define _ret(rn) branchreg(2, 31, 0, 0, rn)

#define eq 0
#define ne 1
#define cs 2
#define cc 3
#define mi 4
#define pl 5
#define vs 6
#define vc 7
#define hi 8
#define ls 9
#define ge 10
#define lt 11
#define gt 12
#define le 13
#define al 14
#define nv 15
#define hs cs
#define lo cc

#define beq(l) b(eq, l)
#define bne(l) b(ne, l)
#define bcs(l) b(cs, l)
#define bcc(l) b(cc, l)
#define bmi(l) b(mi, l)
#define bpl(l) b(pl, l)
#define bvs(l) b(vs, l)
#define bvc(l) b(vc, l)
#define bhi(l) b(hi, l)
#define bls(l) b(ls, l)
#define bge(l) b(ge, l)
#define blt(l) b(lt, l)
#define bgt(l) b(gt, l)
#define ble(l) b(le, l)
#define bal(l) b(al, l)
#define bnv(l) b(nv, l)
#define bhs(l) b(hs, l)
#define blo(l) b(lo, l)

#define Label(l, ...) rasLabel l = Lnew(__VA_ARGS__)
#define Lnew(...) __VA_IF(_Lnewext(__VA_ARGS__), _Lnew(), __VA_ARGS__)
#define _Lnew() rasDeclareLabel(RAS_CTX_VAR)
#define _Lnewext(addr) rasDefineLabelExternal(_Lnew(), addr)
#define L(l) rasDefineLabel(RAS_CTX_VAR, l)
#define Lext(l, addr) rasDefineLabelExternal(l, addr)

#define reg(n) ((rasReg) {n})

#define r0 reg(0)
#define r1 reg(1)
#define r2 reg(2)
#define r3 reg(3)
#define r4 reg(4)
#define r5 reg(5)
#define r6 reg(6)
#define r7 reg(7)
#define r8 reg(8)
#define r9 reg(9)
#define r10 reg(10)
#define r11 reg(11)
#define r12 reg(12)
#define r13 reg(13)
#define r14 reg(14)
#define r15 reg(15)
#define r16 reg(16)
#define r17 reg(17)
#define r18 reg(18)
#define r19 reg(19)
#define r20 reg(20)
#define r21 reg(21)
#define r22 reg(22)
#define r23 reg(23)
#define r24 reg(24)
#define r25 reg(25)
#define r26 reg(26)
#define r27 reg(27)
#define r28 reg(28)
#define r29 reg(29)
#define r30 reg(30)

#define zr ((rasReg) {31, 0})
#define sp ((rasReg) {31, 1})

#define fp r29
#define lr r30

#endif