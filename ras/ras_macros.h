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
#define __FORCE_INT(op)                                                        \
    _Generic(op,                                                               \
        signed char: op,                                                       \
        unsigned char: op,                                                     \
        unsigned short: op,                                                    \
        signed short: op,                                                      \
        unsigned int: op,                                                      \
        signed int: op,                                                        \
        unsigned long: op,                                                     \
        signed long: op,                                                       \
        unsigned long long: op,                                                \
        signed long long: op,                                                  \
        default: *(int*) _ras_invalid_argument_type)
#define __FORCE(type, val)                                                     \
    _Generic(val, type: val, default: *(type*) _ras_invalid_argument_type)

#define word(w) __EMIT(Word, w)
#define dword(d)                                                               \
    _Generic(d,                                                                \
        rasLabel: __EMIT(AbsAddr, __FORCE(rasLabel, d)),                       \
        default: __EMIT(Dword, __FORCE_INT(d)))

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
            rasReg: __EMIT(PseudoAddSubImm, sf, op, s, rd, rn,                 \
                           __FORCE_INT(op2), __FORCE(rasReg, mod)),            \
            default: __EMIT(AddSubImm, sf, op, s, __FORCE(rasShift, mod),      \
                            __FORCE_INT(op2), rn, rd)))

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

#define addsubcarry(sf, op, s, rd, rn, rm)                                     \
    __EMIT(AddSubCarry, sf, op, s, rm, rn, rd)

#define adc(rd, rn, rm) addsubcarry(0, 0, 0, rd, rn, rm)
#define sbc(rd, rn, rm) addsubcarry(0, 1, 0, rd, rn, rm)
#define adcs(rd, rn, rm) addsubcarry(0, 0, 1, rd, rn, rm)
#define sbcs(rd, rn, rm) addsubcarry(0, 1, 1, rd, rn, rm)
#define adcx(rd, rn, rm) addsubcarry(1, 0, 0, rd, rn, rm)
#define sbcx(rd, rn, rm) addsubcarry(1, 1, 0, rd, rn, rm)
#define adcsx(rd, rn, rm) addsubcarry(1, 0, 1, rd, rn, rm)
#define sbcsx(rd, rn, rm) addsubcarry(1, 1, 1, rd, rn, rm)

#define __CINV(n, v) ((n) ? ~(v) : (v))

#define logical(sf, opc, n, rd, rn, op2, ...)                                  \
    _logical(sf, opc, n, rd, rn, op2, __VA_DFL(lsl(0), __VA_ARGS__))
#define _logical(sf, opc, n, rd, rn, op2, mod)                                 \
    _Generic(op2,                                                              \
        rasReg: __EMIT(LogicalReg, sf, opc, n, __FORCE(rasShift, mod),         \
                       __FORCE(rasReg, op2), rn, rd),                          \
        default: _Generic(mod,                                                 \
            rasReg: __EMIT(PseudoLogicalImm, sf, opc, rd, rn,                  \
                           __CINV(n, __FORCE_INT(op2)), __FORCE(rasReg, mod)), \
            default: __EMIT(LogicalImm, sf, opc, __CINV(n, __FORCE_INT(op2)),  \
                            rn, rd)))

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

#define dataproc1source(sf, s, opcode2, opcode, rd, rn)                        \
    __EMIT(DataProc1Source, sf, s, opcode2, opcode, rn, rd)

#define rbit(rd, rn) dataproc1source(0, 0, 0, 0, rd, rn)
#define rev16(rd, rn) dataproc1source(0, 0, 0, 1, rd, rn)
#define rev(rd, rn) dataproc1source(0, 0, 0, 2, rd, rn)
#define clz(rd, rn) dataproc1source(0, 0, 0, 4, rd, rn)
#define cls(rd, rn) dataproc1source(0, 0, 0, 5, rd, rn)
#define rbitx(rd, rn) dataproc1source(1, 0, 0, 0, rd, rn)
#define rev16x(rd, rn) dataproc1source(1, 0, 0, 1, rd, rn)
#define rev32x(rd, rn) dataproc1source(1, 0, 0, 2, rd, rn)
#define revx(rd, rn) dataproc1source(1, 0, 0, 3, rd, rn)
#define clzx(rd, rn) dataproc1source(1, 0, 0, 4, rd, rn)
#define clsx(rd, rn) dataproc1source(1, 0, 0, 5, rd, rn)
#define rev32(rd, rn) rev32x(rd, rn)

#define dataproc2source(sf, s, opcode, rd, rn, rm)                             \
    __EMIT(DataProc2Source, sf, s, rm, opcode, rn, rd)

#define udiv(rd, rn, rm) dataproc2source(0, 0, 2, rd, rn, rm)
#define sdiv(rd, rn, rm) dataproc2source(0, 0, 3, rd, rn, rm)
#define lslv(rd, rn, rm) dataproc2source(0, 0, 8, rd, rn, rm)
#define lsrv(rd, rn, rm) dataproc2source(0, 0, 9, rd, rn, rm)
#define asrv(rd, rn, rm) dataproc2source(0, 0, 10, rd, rn, rm)
#define rorv(rd, rn, rm) dataproc2source(0, 0, 11, rd, rn, rm)
#define udivx(rd, rn, rm) dataproc2source(1, 0, 2, rd, rn, rm)
#define sdivx(rd, rn, rm) dataproc2source(1, 0, 3, rd, rn, rm)
#define lslvx(rd, rn, rm) dataproc2source(1, 0, 8, rd, rn, rm)
#define lsrvx(rd, rn, rm) dataproc2source(1, 0, 9, rd, rn, rm)
#define asrvx(rd, rn, rm) dataproc2source(1, 0, 10, rd, rn, rm)
#define rorvx(rd, rn, rm) dataproc2source(1, 0, 11, rd, rn, rm)

#define dataproc3source(sf, op54, op31, o0, rd, rn, rm, ra)                    \
    __EMIT(DataProc3Source, sf, op54, op31, rm, o0, ra, rn, rd)

#define madd(rd, rn, rm, ra) dataproc3source(0, 0, 0, 0, rd, rn, rm, ra)
#define msub(rd, rn, rm, ra) dataproc3source(0, 0, 0, 1, rd, rn, rm, ra)
#define maddx(rd, rn, rm, ra) dataproc3source(1, 0, 0, 0, rd, rn, rm, ra)
#define msubx(rd, rn, rm, ra) dataproc3source(1, 0, 0, 1, rd, rn, rm, ra)
#define smaddl(rd, rn, rm, ra) dataproc3source(1, 0, 1, 0, rd, rn, rm, ra)
#define umaddl(rd, rn, rm, ra) dataproc3source(1, 0, 5, 0, rd, rn, rm, ra)
#define mul(rd, rn, rm) madd(rd, rn, rm, zr)
#define mneg(rd, rn, rm) msub(rd, rn, rm, zr)
#define mulx(rd, rn, rm) maddx(rd, rn, rm, zr)
#define mnegx(rd, rn, rm) msubx(rd, rn, rm, zr)
#define smull(rd, rn, rm) smaddl(rd, rn, rm, zr)
#define umull(rd, rn, rm) umaddl(rd, rn, rm, zr)

#define condselect(sf, op, s, op2, rd, rn, rm, cond)                           \
    __EMIT(CondSelect, sf, op, s, rm, cond, op2, rn, rd)

#define csel(rd, rn, rm, cond) condselect(0, 0, 0, 0, rd, rn, rm, cond)
#define csinc(rd, rn, rm, cond) condselect(0, 0, 0, 1, rd, rn, rm, cond)
#define csinv(rd, rn, rm, cond) condselect(0, 1, 0, 0, rd, rn, rm, cond)
#define csneg(rd, rn, rm, cond) condselect(0, 1, 0, 1, rd, rn, rm, cond)
#define cselx(rd, rn, rm, cond) condselect(1, 0, 0, 0, rd, rn, rm, cond)
#define csincx(rd, rn, rm, cond) condselect(1, 0, 0, 1, rd, rn, rm, cond)
#define csinvx(rd, rn, rm, cond) condselect(1, 1, 0, 0, rd, rn, rm, cond)
#define csnegx(rd, rn, rm, cond) condselect(1, 1, 0, 1, rd, rn, rm, cond)
#define cmov(rd, rm, cond) csel(rd, rm, rd, cond)
#define cset(rd, cond) csinc(rd, zr, zr, (cond) ^ 1)
#define csetm(rd, cond) csinv(rd, zr, zr, (cond) ^ 1)
#define cinv(rd, rm, cond) csinv(rd, rm, rm, (cond) ^ 1)
#define cinc(rd, rm, cond) csinc(rd, rm, rm, (cond) ^ 1)
#define cneg(rd, rm, cond) csneg(rd, rm, rm, (cond) ^ 1)
#define cmovx(rd, rm, cond) cselx(rd, rm, rd, (cond) ^ 1)
#define csetx(rd, cond) csincx(rd, zr, zr, (cond) ^ 1)
#define csetmx(rd, cond) csinvx(rd, zr, zr, (cond) ^ 1)
#define cinvx(rd, rm, cond) csinvx(rd, rm, rm, (cond) ^ 1)
#define cincx(rd, rm, cond) csincx(rd, rm, rm, (cond) ^ 1)
#define cnegx(rd, rm, cond) csnegx(rd, rm, rm, (cond) ^ 1)

#define extract(sf, op21, n, o0, rd, rn, rm, imms)                             \
    __EMIT(Extract, sf, op21, n, o0, rm, imms, rn, rd)

#define extr(rd, rn, rm, imms) extract(0, 0, 0, 0, rd, rn, rm, imms)
#define extrx(rd, rn, rm, imms) extract(1, 0, 1, 0, rd, rn, rm, imms)

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

#define pcreladdr(op, rd, l) __EMIT(PCRelAddr, op, l, rd)

#define adr(rd, l) pcreladdr(0, rd, l)
#define adrp(rd, l) pcreladdr(1, rd, l)
#define adrl(rd, l) __EMIT(PseudoPCRelAddrLong, rd, l)

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
        rasReg: __EMIT(PseudoMovReg, 0, rd, __FORCE(rasReg, op2)),             \
        default: __EMIT(PseudoMovImm, 0, rd, __FORCE_INT(op2)))
#define movx(rd, op2)                                                          \
    _Generic(op2,                                                              \
        rasReg: __EMIT(PseudoMovReg, 1, rd, __FORCE(rasReg, op2)),             \
        default: __EMIT(PseudoMovImm, 1, rd, __FORCE_INT(op2)))

#define __MAKE_EXT(s)                                                          \
    _Generic(s,                                                                \
        rasShift: __EXT_OF_SHIFT(__FORCE(rasShift, s)),                        \
        rasExtend: s,                                                          \
        default: *(rasExtend*) _ras_invalid_argument_type)
#define __EXT_OF_SHIFT(s) ((rasExtend) {s.amt, 3, s.type != 0 || s.amt > 4})

#define loadstore(size, opc, rt, amod) _loadstore(size, opc, rt, __ID amod)
#define _loadstore(size, opc, rt, amod) __loadstore(size, opc, rt, amod)
#define __loadstore(size, opc, rt, rn, ...)                                    \
    ___loadstore(size, opc, rt, rn, __VA_DFL(0, __VA_ARGS__))
#define ___loadstore(size, opc, rt, rn, off)                                   \
    ____loadstore(size, opc, rt, rn, off)
#define ____loadstore(size, opc, rt, rn, off, ...)                             \
    _Generic(off,                                                              \
        rasReg: rasEmitLoadStoreRegOff,                                        \
        default: rasEmitLoadStoreImmOff)(                                      \
        RAS_CTX_VAR, size, opc, off,                                           \
        _Generic(off,                                                          \
            rasReg: __MAKE_EXT(__VA_DFL(uxtx(), __VA_ARGS__)),                 \
            default: __VA_DFL(0, __VA_ARGS__)),                                \
        rn, rt)

#define strb(rt, amod) loadstore(0, 0, rt, amod)
#define ldrb(rt, amod) loadstore(0, 1, rt, amod)
#define ldrsbx(rt, amod) loadstore(0, 2, rt, amod)
#define ldrsb(rt, amod) loadstore(0, 3, rt, amod)
#define strh(rt, amod) loadstore(1, 0, rt, amod)
#define ldrh(rt, amod) loadstore(1, 1, rt, amod)
#define ldrshx(rt, amod) loadstore(1, 2, rt, amod)
#define ldrsh(rt, amod) loadstore(1, 3, rt, amod)
#define str(rt, amod) loadstore(2, 0, rt, amod)
#define ldr(rt, amod) loadstore(2, 1, rt, amod)
#define ldrswx(rt, amod) loadstore(2, 2, rt, amod)
#define strx(rt, amod) loadstore(3, 0, rt, amod)
#define ldrx(rt, amod) loadstore(3, 1, rt, amod)
#define ldrsw(rt, amod) ldrswx(rt, amod)

#define loadliteral(opc, rt, l) __EMIT(LoadLiteral, opc, l, rt)

#define ldrl(rt, l) loadliteral(0, rt, l)
#define ldrxl(rt, l) loadliteral(1, rt, l)
#define ldrswxl(rt, l) loadliteral(2, rt, l)
#define ldrswl(rt, l) ldrswxl(rt, l)

#define loadstorepair(opc, l, rt, rt2, amod)                                   \
    _loadstorepair(opc, l, rt, rt2, __ID amod)
#define _loadstorepair(opc, l, rt, rt2, amod)                                  \
    __loadstorepair(opc, l, rt, rt2, amod)
#define __loadstorepair(opc, l, rt, rt2, rn, ...)                              \
    ___loadstorepair(opc, l, rt, rt2, rn, __VA_DFL(0, __VA_ARGS__))
#define ___loadstorepair(opc, l, rt, rt2, rn, off)                             \
    ____loadstorepair(opc, l, rt, rt2, rn, off)
#define ____loadstorepair(opc, l, rt, rt2, rn, off, ...)                       \
    __EMIT(LoadStorePair, opc, __VA_DFL(2, __VA_ARGS__), l, off, rt2, rn, rt)

#define stp(rt, rt2, amod) loadstorepair(0, 0, rt, rt2, amod)
#define ldp(rt, rt2, amod) loadstorepair(0, 1, rt, rt2, amod)
#define ldpswx(rt, rt2, amod) loadstorepair(1, 1, rt, rt2, amod)
#define stpx(rt, rt2, amod) loadstorepair(2, 0, rt, rt2, amod)
#define ldpx(rt, rt2, amod) loadstorepair(2, 1, rt, rt2, amod)
#define push(rt, rt2) stpx(rt, rt2, (sp, -0x10, pre))
#define pop(rt, rt2) ldpx(rt, rt2, (sp, 0x10, post))

#define post 1
#define pre 3

#define branchuncondimm(op, l) __EMIT(BranchUncondImm, op, l)
#define branchcondimm(c, o0, l) __EMIT(BranchCondImm, l, o0, c)

#define b(l, ...)                                                              \
    __VA_IF(branchcondimm(l, 0, __VA_ARGS__), branchuncondimm(0, l),           \
            __VA_ARGS__)
#define bl(l) branchuncondimm(1, l)

#define branchcompimm(sf, op, rt, l) __EMIT(BranchCompImm, sf, op, l, rt)

#define cbz(rt, l) branchcompimm(0, 0, rt, l)
#define cbnz(rt, l) branchcompimm(0, 1, rt, l)
#define cbzx(rt, l) branchcompimm(1, 0, rt, l)
#define cbnzx(rt, l) branchcompimm(1, 1, rt, l)

#define branchreg(opc, op2, op3, op4, rn)                                      \
    __EMIT(BranchReg, opc, op2, op3, rn, op4)

#define br(rn) branchreg(0, 31, 0, 0, rn)
#define blr(rn) branchreg(1, 31, 0, 0, rn)
#define ret(...) branchreg(2, 31, 0, 0, __VA_DFL(lr, __VA_ARGS__))

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

#define hint(crm, op2) __EMIT(Hint, crm, op2)
#define nop() hint(0, 0)

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
#define ip1 r16
#define ip2 r17
#define fp r29
#define lr r30
#define zr ((rasReg) {31, 0})
#define sp ((rasReg) {31, 1})

#endif