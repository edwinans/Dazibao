CC=gcc
CFLAGS= -g -Wall -I$(IDIR)

IDIR =../include

ODIR=obj

LIBS=-lm

_DEPS = network.h tlv_makers.h tlv_handlers.h hash.h 
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = tlv_makers.o tlv_handlers.o core.o hash.o sha224-256.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: core test

core: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

test: tlv_makers.o test.o hash.o sha224-256.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ *.o* core test $(INCDIR)/*~ 