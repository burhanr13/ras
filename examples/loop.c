#include <stdio.h>
#include <stdlib.h>

#define RAS_MACROS
#define RAS_DEFAULT_SUFFIX w
#include "ras/ras.h"

int main() {
    rasBlock* ctx = rasCreate(16384);

    Label(lend);
    Label(lloop);

    push(fp, lr);

    mov(r1, 0);

    L(lloop);
    cmp(r0, 0);
    beq(lend);

    add(r1, r1, r0);
    sub(r0, r0, 1);

    b(lloop);
    L(lend);

    add(r0, r1, 0);

    pop(fp, lr);
    ret();

    rasReady(ctx);

    int (*f)(int) = rasGetCode(ctx);
    printf("%d\n", f(10));

    rasDestroy(ctx);
}