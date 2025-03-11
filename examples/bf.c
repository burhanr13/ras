#include <stdio.h>
#include <stdlib.h>

#define RAS_MACROS
#include "ras/ras.h"

char data[65536];

int main(int argc, char** argv) {
    rasBlock* ctx = rasCreate(16384);

    char* p;
    if (argc < 2) {
        // hello world from wikipedia
        p = "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++.."
            "+++.>>.<-.<.+++.------.--------.>>+.>++.";
    } else {
        FILE* f = fopen(argv[1], "r");
        fseek(f, 0, SEEK_END);
        int len = ftell(f);
        rewind(f);
        p = malloc(len + 1);
        fread(p, 1, len, f);
    }

    rasLabel labs[100][2];
    int top = 0;

    char c;
    // r25: data ptr
    // r26: idx
    // r27: putchar ptr
    // r28: getchar ptr
    push(fp, lr);
    push(r27, r28);
    push(r25, r26);
    movx(r26, r0);
    movx(r27, 0);
    adrl(r28, Lnew(putchar));
    adrl(r29, Lnew(getchar));
    while ((c = *p++)) {
        switch (c) {
            case '>': {
                int ct = 1;
                while (*p == '>') {
                    ct++;
                    p++;
                }
                addx(r27, r27, ct);
                uxth(r27, r27);
                break;
            }
            case '<': {
                int ct = 1;
                while (*p == '<') {
                    ct++;
                    p++;
                }
                subx(r27, r27, ct);
                uxth(r27, r27);
                break;
            }
            case '+': {
                int ct = 1;
                while (*p == '+') {
                    ct++;
                    p++;
                }
                ldrb(r0, (r26, r27));
                addw(r0, r0, ct);
                strb(r0, (r26, r27));
                break;
            }
            case '-': {
                int ct = 1;
                while (*p == '-') {
                    ct++;
                    p++;
                }
                ldrb(r0, (r26, r27));
                subw(r0, r0, ct);
                strb(r0, (r26, r27));
                break;
            }
            case '.': {
                ldrb(r0, (r26, r27));
                blr(r28);
                break;
            }
            case ',': {
                blr(r29);
                strb(r0, (r26, r27));
                break;
            }
            case '[': {
                top++;
                labs[top][0] = Lnew();
                labs[top][1] = Lnew();
                ldrb(r0, (r26, r27));
                cbzw(r0, labs[top][1]);
                L(labs[top][0]);
                break;
            }
            case ']': {
                ldrb(r0, (r26, r27));
                cbnzw(r0, labs[top][0]);
                L(labs[top][1]);
                top--;
                break;
            }
        }
    }

    pop(r25, r26);
    pop(r27, r28);
    pop(fp, lr);
    ret();

    rasReady(ctx);

    void (*f)(char*) = rasGetCode(ctx);
    f(data);

    rasDestroy(ctx);
}