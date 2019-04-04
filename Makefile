IDIR=./include
CC=gcc

CFLAGSDEBUG=-I$(IDIR) -g
CFLAGSTEST=-I$(IDIR) -ansi -g -Werror -Wextra -Wformat=2 -Wjump-misses-init -Wlogical-op -Wpedantic -Wshadow
CFLAGS=-I$(IDIR) -ansi -g -Wall -Werror -Wextra -Wformat=2 -Wjump-misses-init -Wlogical-op -Wpedantic -Wshadow

ODIR=obj

_DEPS = common.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = common.o ls.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGSTEST)

ls: $(OBJ) 
	$(CC) -o $@ $^ $(CFLAGSTEST)	

.PHONY: clean
clean:
	rm -f $(ODIR)/* *~ core $(IDIR)/*~

