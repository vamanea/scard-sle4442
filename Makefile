CC=cc
CFLAGS=-pthread -I/usr/include/PCSC -I/usr/local/include/ -I. -c
LDFLAGS=-lpcsclite -lsyncapi
 
all: pcsc_demo.o pcsc_read
 
pcsc_read: pcsc_demo.o
	$(CC) pcsc_demo.o $(LDFLAGS) -o pcsc_read


pcsc_demo.o: pcsc_demo.c
	$(CC) $(CFLAGS) pcsc_demo.c

 
clean:
	rm -rf *.o readSmartCard pcsc_read
