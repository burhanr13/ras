
SRCS := $(filter-out ras.c, $(wildcard *.c))

TARGETS := $(SRCS:%.c=bin/%)

all: $(TARGETS)

bin/%: %.c ras.c ../ras/ras.h ../ras/ras_impl.h ../ras/ras_macros.h
	@mkdir -p bin
	gcc -g -o $@ -I.. $< ras.c

clean:
	rm -rf bin
