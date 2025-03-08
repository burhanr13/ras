
test: test.c ras/ras.c
	gcc -g -o $@ $^

test.c: ras/ras.h ras/ras_macros.h
