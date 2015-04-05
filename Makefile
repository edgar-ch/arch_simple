CC=gcc
CFLAGS=-Wall
LDFLAGS=
SOURCES=arch.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=arch

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXECUTABLE)

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)
