#ifndef __RAS_MACROS_COMMON_H
#define __RAS_MACROS_COMMON_H

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
#define __FORCE(type, val)                                                     \
    _Generic(val, type: val, default: *(type*) _ras_invalid_argument_type)

#define ALIGN(a) rasAlign(RAS_CTX_VAR, a)

#endif