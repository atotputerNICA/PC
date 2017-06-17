CC=gcc
LIBSOCKET=-lnsl
CCFLAGS=-Wall -g
SEL_SRV=httpproxy

all: $(SEL_SRV)

$(SEL_SRV):$(SEL_SRV).c
	$(CC) -o $(SEL_SRV) $(LIBSOCKET) $(SEL_SRV).c

clean:
	rm -f *.o *~
	rm -f $(SEL_SRV)
