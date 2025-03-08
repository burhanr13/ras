#include <stdio.h>
#include <stdlib.h>

#include "ras/ras.h"
#include "ras/ras_macros.h"

int test() {
    return 2;
}

int main() {
    rasBlock* ctx = rasCreate(16384);

    Label(lend);
    Label(lloop);

    str(lr, pre_ptr(sp, -0x10));

    movz(w1, 0);

    L(lloop);
    subs(wzr, w0, 0);
    beq(lend);

    add(w1, w1, w0);
    sub(w0, w0, 1);

    b(lloop);
    L(lend);

    add(w0, w1, 0);
    
    ldr(lr, post_ptr(sp, 0x10));
    word(0xd65f03c0); // ret

    rasReady(ctx);

    int (*f)(int) = rasGetCode(ctx);
    printf("%d\n", f(10));

    rasDestroy(ctx);
}