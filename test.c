#include <stdio.h>
#include <stdlib.h>

#include "ras/ras.h"
#include "ras/ras_macros.h"

int test() {
    return 2;
}

int main() {
    rasBlock* ctx = rasCreate(16384);

    str(lr, pre_ptr(sp, -0x10));
    bl(Lnew(test));
    ldr(lr, post_ptr(sp, 0x10));
    word(0xd65f03c0); // ret

    b(eq, Lnew());
    b(Lnew());

    rasReady(ctx);

    int (*f)() = rasGetCode(ctx);
    printf("%d\n", f());

    rasDestroy(ctx);
}