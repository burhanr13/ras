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

#define __OPTION_SHIFT(...) __VA_IF(__ID(__VA_ARGS__), lsl(0), __VA_ARGS__)

// unfortunately generic requires all branches to be well typed
// solve this by using more generic
#define __OP_IMM(op) _Generic(op, rasReg: 0, default: op)
#define __FORCE(type, val) _Generic(val, type: val, default: (type) {0})
#define __OP_IMML(op) _Generic(op, rasLabel: 0, default: op)

#define word(w) __EMIT(Word, w)
#define dword(d)                                                               \
    _Generic(d,                                                                \
        rasLabel: __EMIT(AbsAddr, __FORCE(rasLabel, d)),                       \
        default: __EMIT(Dword, __OP_IMML(d)))

#define addsub(op, s, rd, rn, op2, ...)                                        \
    _addsub(op, s, rd, rn, op2, __OPTION_SHIFT(__VA_ARGS__))
#define _addsub(op, s, rd, rn, op2, mod)                                       \
    _Generic(op2,                                                              \
        rasReg: _Generic(mod,                                                  \
            rasShift: __EMIT(AddSubShiftedReg, op, s, __FORCE(rasShift, mod),  \
                             __FORCE(rasReg, op2), rn, rd),                    \
            rasExtend: __EMIT(AddSubExtendedReg, op, s,                        \
                              __FORCE(rasExtend, mod), __FORCE(rasReg, op2),   \
                              rn, rd)),                                        \
        default: __EMIT(AddSubImm, op, s, __FORCE(rasShift, mod),              \
                        __OP_IMM(op2), rn, rd))

#define add(rd, rn, op2, ...) addsub(0, 0, rd, rn, op2, __VA_ARGS__)
#define adds(rd, rn, op2, ...) addsub(0, 1, rd, rn, op2, __VA_ARGS__)
#define sub(rd, rn, op2, ...) addsub(1, 0, rd, rn, op2, __VA_ARGS__)
#define subs(rd, rn, op2, ...) addsub(1, 1, rd, rn, op2, __VA_ARGS__)

#define movewide(opc, rd, imm, ...)                                            \
    __EMIT(MoveWide, opc, __OPTION_SHIFT(__VA_ARGS__), imm, rd)

#define movn(rd, imm, ...) movewide(0, rd, imm, __VA_ARGS__)
#define movz(rd, imm, ...) movewide(2, rd, imm, __VA_ARGS__)
#define movk(rd, imm, ...) movewide(3, rd, imm, __VA_ARGS__)

#define __EXT_OF_SHIFT(s)                                                      \
    _Generic(s, rasShift: ((rasExtend) {s.amt, 3, s.type != 0}), rasExtend: s)

#define ptr(rn, ...) _ptr(rn, __VA_IF(__ID(__VA_ARGS__), 0, __VA_ARGS__))
#define _ptr(x, y) __ptr(x, y)
#define __ptr(rn, off, ...)                                                    \
    _Generic(off,                                                              \
        rasReg: ((rasAddrReg) {rn, __FORCE(rasReg, off),                       \
                               __EXT_OF_SHIFT(__OPTION_SHIFT(__VA_ARGS__))}),  \
        default: ((rasAddrImm) {rn, 0, __OP_IMM(off)}))

#define post_ptr(rn, off) ((rasAddrImm) {rn, 1, off})
#define pre_ptr(rn, off) ((rasAddrImm) {rn, 3, off})

#define loadstore(size, opc, rt, amod)                                         \
    _Generic(amod,                                                             \
        rasAddrReg: __EMIT(LoadStoreRegOff, size, opc,                         \
                           __FORCE(rasAddrReg, amod), rt),                     \
        rasAddrImm: __EMIT(LoadStoreImmOff, size, opc,                         \
                           __FORCE(rasAddrImm, amod), rt))

#define strb(rt, amod) loadstore(0, 0, rt, amod)
#define ldrb(rt, amod) loadstore(0, 1, rt, amod)
#define ldrsb(rt, amod) loadstore(0, 2, rt, amod)
#define strh(rt, amod) loadstore(1, 0, rt, amod)
#define ldrh(rt, amod) loadstore(1, 1, rt, amod)
#define ldrsh(rt, amod) loadstore(1, 2, rt, amod)
#define str(rt, amod) loadstore(2, 0, rt, amod)
#define ldr(rt, amod) loadstore(2, 1, rt, amod)
#define ldrsw(rt, amod) loadstore(2, 2, rt, amod)

#define branchuncondimm(op, l) __EMIT(BranchUncondImm, op, l)
#define branchcondimm(c, o0, l) __EMIT(BranchCondImm, l, o0, c)

#define b(l, ...)                                                              \
    __VA_IF(branchcondimm(l, 0, __VA_ARGS__), branchuncondimm(0, l),           \
            __VA_ARGS__)
#define bl(l) branchuncondimm(1, l)

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
#define bge(l) b(ge,l)
#define blt(l) b(lt, l)
#define bgt(l) b(gt, l)
#define ble(l) b(le, l)
#define bal(l) b(al, l)
#define bnv(l) b(nv, l)
#define bhs(l) b(hs, l)
#define blo(l) b(lo, l)

#define lsl(s, ...) ((rasShift) {s, 0})
#define lsr(s, ...) ((rasShift) {s, 1})
#define asr(s, ...) ((rasShift) {s, 2})

#define extend(type, ...)                                                      \
    _extend(type, __VA_IF(__ID(__VA_ARGS__), 0, __VA_ARGS__))
#define _extend(type, s, ...) ((rasExtend) {s, type})

#define uxtb(...) extend(0, __VA_ARGS__)
#define uxth(...) extend(1, __VA_ARGS__)
#define uxtw(...) extend(2, __VA_ARGS__)
#define uxtx(...) extend(3, __VA_ARGS__)
#define sxtb(...) extend(4, __VA_ARGS__)
#define sxth(...) extend(5, __VA_ARGS__)
#define sxtw(...) extend(6, __VA_ARGS__)
#define sxtx(...) extend(7, __VA_ARGS__)

#define Label(l, ...) rasLabel l = Lnew(__VA_ARGS__)
#define Lnew(...) __VA_IF(_Lnewext(__VA_ARGS__), _Lnew(), __VA_ARGS__)
#define _Lnew() rasDeclareLabel(RAS_CTX_VAR)
#define _Lnewext(addr) rasDefineLabelExternal(_Lnew(), addr)
#define L(l) rasDefineLabel(RAS_CTX_VAR, l)

#define wreg(n) ((rasReg) {n, 0})
#define xreg(n) ((rasReg) {n, 1})

#define w0 wreg(0)
#define w1 wreg(1)
#define w2 wreg(2)
#define w3 wreg(3)
#define w4 wreg(4)
#define w5 wreg(5)
#define w6 wreg(6)
#define w7 wreg(7)
#define w8 wreg(8)
#define w9 wreg(9)
#define w10 wreg(10)
#define w11 wreg(11)
#define w12 wreg(12)
#define w13 wreg(13)
#define w14 wreg(14)
#define w15 wreg(15)
#define w16 wreg(16)
#define w17 wreg(17)
#define w18 wreg(18)
#define w19 wreg(19)
#define w20 wreg(20)
#define w21 wreg(21)
#define w22 wreg(22)
#define w23 wreg(23)
#define w24 wreg(24)
#define w25 wreg(25)
#define w26 wreg(26)
#define w27 wreg(27)
#define w28 wreg(28)
#define w29 wreg(29)
#define w30 wreg(30)

#define wzr ((rasReg) {31, 0, 0})
#define wsp ((rasReg) {31, 0, 1})

#define x0 xreg(0)
#define x1 xreg(1)
#define x2 xreg(2)
#define x3 xreg(3)
#define x4 xreg(4)
#define x5 xreg(5)
#define x6 xreg(6)
#define x7 xreg(7)
#define x8 xreg(8)
#define x9 xreg(9)
#define x10 xreg(10)
#define x11 xreg(11)
#define x12 xreg(12)
#define x13 xreg(13)
#define x14 xreg(14)
#define x15 xreg(15)
#define x16 xreg(16)
#define x17 xreg(17)
#define x18 xreg(18)
#define x19 xreg(19)
#define x20 xreg(20)
#define x21 xreg(21)
#define x22 xreg(22)
#define x23 xreg(23)
#define x24 xreg(24)
#define x25 xreg(25)
#define x26 xreg(26)
#define x27 xreg(27)
#define x28 xreg(28)
#define x29 xreg(29)
#define x30 xreg(30)

#define xzr ((rasReg) {31, 1, 0})
#define sp ((rasReg) {31, 1, 1})

#define fp x29
#define lr x30

#endif