
SRCS := $(wildcard *.c)

TARGETS := $(SRCS:%.c=bin/%)

all: $(TARGETS)

bin/%: %.c ../ras/ras.c ../ras/ras.h ../ras/ras_macros.h
	@mkdir -p bin
	gcc -g -o $@ -I.. $< ../ras/ras.c

clean:
	rm -rf bin
