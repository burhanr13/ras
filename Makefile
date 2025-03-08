
test: test.c ras/ras.c ras/ras.h ras/ras_macros.h
	gcc -g -o $@ $< ras/ras.c
