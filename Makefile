make: main.c
	gcc -Wall memsim.c -o memsim

run:
	./memsim

clean:
	rm memsim

.SILENT: clean make run
