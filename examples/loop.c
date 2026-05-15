#include <stdio.h>
#include <stdlib.h>

#define RAS_DEFAULT_SUFFIX W
#include "ras/ras_a64.h"

int main() {
    rasBlock* ctx = rasCreate(16384);

    LABEL(lend);
    LABEL(lloop);

    PUSH(FP, LR);
    MOV(R1, 0);
    L(lloop);
    CMP(R0, 0);
    BEQ(lend);
    ADD(R1, R1, R0);
    SUB(R0, R0, 1);
    B(lloop);
    L(lend);
    MOV(R0, R1);
    POP(FP, LR);
    RET();

    rasReady(ctx);

    int (*f)(int) = rasGetCode(ctx);
    printf("%d\n", f(10));

    rasDestroy(ctx);
}