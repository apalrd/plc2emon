PROG = plc2emon
SOURCES = $(PROG).c
CFLAGS = -O -W -std=c99 $(CFLAGS_EXTRA)

all: $(SOURCES)
	$(CC) -o $(PROG) $(SOURCES) $(CFLAGS)

clean:
	rm -rf *.gc* *.dSYM *.exe *.obj *.o a.out $(PROG)
