
SRCS := $(filter-out ras.c, $(wildcard *.c))

TARGETS := $(SRCS:%.c=bin/%)

all: $(TARGETS)

bin/%: %.c
	@mkdir -p bin
	gcc -g -o $@ -I.. -DRAS_AUTOGROW $< ../ras/ras.c ../ras/ras_a64.c

clean:
	rm -rf bin
