CC=mpicc
CFLAGS=-Wall -fPIC

.PHONY: clean default

default: mpi-papi-collector.so


mpi-papi-collector.so: mpi-papi-collector.o
	$(CC) $(CFLAGS) -shared -lpapi $< -o $@


clean:
	$(RM) *.so
	$(RM) *.o
