bin/shell: src/main.c src/shell.c src/execute.c
	gcc -Iinclude src/main.c src/shell.c src/execute.c -o bin/shell -lreadline

clean:
	rm -f shell
