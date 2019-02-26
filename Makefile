make: memsim.c
	gcc -Wall memsim.c -o memsim

run:
	./memsim $(TF) $(NF) $(AL) $(MD) #Usage: make run TF=Traces/foo.trace NF=nframes AL=algorithm MD=mode

clean:
	rm memsim

.SILENT: clean make run
