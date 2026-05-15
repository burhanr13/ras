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
    // R25: data ptr
    // R26: idx
    // R27: putchar ptr
    // R28: getchar ptr
    PUSH(FP, LR);
    PUSH(R27, R28);
    PUSH(R25, R26);
    MOVX(R26, R0);
    MOVX(R27, 0);
    ADRL(R28, LNEW(putchar));
    ADRL(R29, LNEW(getchar));
    while ((c = *p++)) {
        switch (c) {
            case '>': {
                int ct = 1;
                while (*p == '>') {
                    ct++;
                    p++;
                }
                ADDX(R27, R27, ct);
                UXTH(R27, R27);
                break;
            }
            case '<': {
                int ct = 1;
                while (*p == '<') {
                    ct++;
                    p++;
                }
                SUBX(R27, R27, ct);
                UXTH(R27, R27);
                break;
            }
            case '+': {
                int ct = 1;
                while (*p == '+') {
                    ct++;
                    p++;
                }
                LDRB(R0, (R26, R27));
                ADDW(R0, R0, ct);
                STRB(R0, (R26, R27));
                break;
            }
            case '-': {
                int ct = 1;
                while (*p == '-') {
                    ct++;
                    p++;
                }
                LDRB(R0, (R26, R27));
                SUBW(R0, R0, ct);
                STRB(R0, (R26, R27));
                break;
            }
            case '.': {
                LDRB(R0, (R26, R27));
                BLR(R28);
                break;
            }
            case ',': {
                BLR(R29);
                STRB(R0, (R26, R27));
                break;
            }
            case '[': {
                top++;
                labs[top][0] = LNEW();
                labs[top][1] = LNEW();
                LDRB(R0, (R26, R27));
                CBZW(R0, labs[top][1]);
                L(labs[top][0]);
                break;
            }
            case ']': {
                LDRB(R0, (R26, R27));
                CBNZW(R0, labs[top][0]);
                L(labs[top][1]);
                top--;
                break;
            }
        }
    }

    POP(R25, R26);
    POP(R27, R28);
    POP(FP, LR);
    RET();

    rasReady(ctx);

    void (*f)(char*) = rasGetCode(ctx);
    f(data);

    rasDestroy(ctx);
}