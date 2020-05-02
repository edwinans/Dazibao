CC=gcc
CFLAGS= -g -Wall -std=c11 -I$(IDIR)

IDIR =../include

ODIR=obj

LIBS=-lm

_DEPS = tlv.h hash.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = tlv.o test.o hash.o sha224-256.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core test $(INCDIR)/*~ 