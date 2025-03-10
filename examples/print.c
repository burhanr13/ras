#include <stdio.h>
#include <stdlib.h>

#define RAS_MACROS
#include "ras/ras.h"

char message[] = "this message was printed by the jit\n";

int main() {
    rasBlock* ctx = rasCreate(16384);

    push(fp, lr);
    adr(r0, Lnew(message));
    adrl(ip1, Lnew(printf));
    blr(ip1);
    pop(fp, lr);
    ret();

    rasReady(ctx);

    void (*f)() = rasGetCode(ctx);
    f();

    rasDestroy(ctx);
}