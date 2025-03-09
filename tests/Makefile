
bin/tests: tests.c ../ras/ras.c test_input.txt ../ras/ras.h ../ras/ras_macros.h
	@mkdir -p bin
	gcc -g -o $@ -I/opt/homebrew/include -I.. $< ../ras/ras.c -L/opt/homebrew/lib -lcapstone

clean:
	rm -rf bin
