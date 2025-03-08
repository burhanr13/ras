
ras_test: ras_test.c ../ras/ras.c
	gcc -g -o $@ -I/opt/homebrew/include $^ -L/opt/homebrew/lib -lcapstone

ras_test.c: test_input.txt ../ras/ras.h ../ras/ras_macros.h
