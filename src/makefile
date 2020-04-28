# CC=gcc
# CFLAGS= -g -Wall -std=c11 -I -o 

# .PHONY:all 
# all: test

# test: test.o tlv.o
# 	$(CC) -o test test.o tlv.o -I./src 

# .PHONY:clean
# clean:
# 	@rm -rf *.o

IDIR =../include
CC=gcc
CFLAGS= -g -Wall -std=c11 -I$(IDIR)

ODIR=obj

LIBS=-lm

_DEPS = tlv.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = tlv.o test.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 