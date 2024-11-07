OBJECTS=main.o mem.o pic.o chipview.o aegl.o
CFLAGS=-Wall -Wextra
LDFLAGS=-lcurses

all: pic16chu

pic16chu: ${OBJECTS}
	gcc -o pic16chu $^ ${LDFLAGS}

main.o: main.c
	gcc -c $^ ${CFLAGS}

pic.o: pic.c
	gcc -c $^ ${CFLAGS}

mem.o: mem.c
	gcc -c $^ ${CFLAGS}

chipview.o: chipview.c
	gcc -c $^ ${CFLAGS}

aegl.o: aegl.c
	gcc -c $^ ${CFLAGS}

.PHONY: clean
clean:
	rm -f *.o pic16chu

