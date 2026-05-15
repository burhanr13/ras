
bin/tests: tests.c test_input.txt
	@mkdir -p bin
	gcc -g -o $@ -I/opt/homebrew/include -I.. $< ../ras/ras.c -L/opt/homebrew/lib -lcapstone

clean:
	rm -rf bin
