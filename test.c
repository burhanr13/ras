#include <stdio.h>
#include <stdlib.h>

#include "ras/ras.h"
#include "ras/ras_macros.h"

int main() {
    rasBlock* ctx = rasCreate(16384);

    ldr(w0, ptr(x1, 0xff0));

    word(0xd65f03c0); // ret

    rasReady(ctx);

    int (*f)() = rasGetCode(ctx);

    printf("%08x\n", f());

    rasDestroy(ctx);
}