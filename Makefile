MAIN = pa6
SOURCES = main.c forklib.c ipc.c cs.c
HEADERS = main.h forklib.h
CC = clang

OBJECTS = $(SOURCES:.c=.o)

LOGS = pipes.log events.log

all: $(MAIN) $(HEADERS)

$(MAIN): $(SOURCES)
	$(CC) --std=c99 -Wall -pedantic -o $(MAIN) *.c -L. -lruntime

clean:
	$(RM) $(OBJECTS)
	$(RM) $(MAIN)
	$(RM) $(LOGS)

.PHONY: all clean
