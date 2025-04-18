#include <capstone/capstone.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAS_MACROS
#define RAS_CTX_VAR testCode
#define RAS_DEFAULT_SUFFIX w
#include "ras/ras.h"

void errorCb(rasError err) {
    fprintf(stderr, "%s\n", rasErrorStrings[err]);
    abort();
}

int main() {
    rasSetErrorCallback((rasErrorCallback) errorCb, NULL);

    rasBlock* testCode = rasCreate(16384);

#include "test_input.txt"

    rasReady(testCode);

    FILE* testin = fopen("test_expected.txt", "r");
    FILE* testout = fopen("test_actual.txt", "w");
    if (!testin || !testout) exit(1);

    int failct = 0;

    csh handle;
    cs_insn* insn;
    cs_open(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN, &handle);;
    // make sure capstone does not stop on invalid instructions
    cs_option(handle, CS_OPT_SKIPDATA, CS_OPT_ON);

    size_t count = cs_disasm(handle, rasGetCode(testCode), rasGetSize(testCode),
                             0, 0, &insn);
    for (size_t i = 0; i < count; i++) {
        char expected[1000];
        fgets(expected, 1000, testin);
        char actual[1000];
        snprintf(actual, 1000, "%s %s\n", insn[i].mnemonic, insn[i].op_str);

        if (strcmp(expected, actual)) {
            failct++;
            fprintf(stderr, "expected:%sactual:%s\n", expected, actual);
        }

        fprintf(testout, "%s", actual);
    }
    cs_free(insn, count);
    cs_close(&handle);

    fclose(testin);
    fclose(testout);

    rasDestroy(testCode);

    return failct;
}