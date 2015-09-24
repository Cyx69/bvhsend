PROG = bvhsend
SOURCES = $(PROG).c
OBJECTS = $(SOURCES:.c=.o)
CC = gcc
CFLAGS = -s -c -Werror
LDFLAGS = 
#-mno-cygwin

all: $(SOURCES) makefile $(PROG)

$(PROG): $(OBJECTS)
	$(CC) $(LDFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(PROG) $(PROG).exe $(OBJECTS) *.stackdump
