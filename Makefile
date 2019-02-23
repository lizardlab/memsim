make: memsim.c
	gcc -Wall memsim.c -o memsim

run:
	./memsim $(TF) #Usage: make run TF=Traces/foo.trace

clean:
	rm memsim

.SILENT: clean make run
