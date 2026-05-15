#include <stdio.h>
#include <stdlib.h>

#define RAS_MACROS
#include "ras/ras.h"

char message[] = "this message was printed by the jit\n";

int main() {
    rasBlock* ctx = rasCreate(16384);

    PUSH(FP, LR);
    ADR(R0, LNEW(message));
    ADRL(IP1, LNEW(printf));
    BLR(IP1);
    POP(FP, LR);
    RET();

    rasReady(ctx);

    void (*f)() = rasGetCode(ctx);
    f();

    rasDestroy(ctx);
}