# Compiler, initially GCC however can be changed if needed
CC       = gcc
CFLAGS   = -Wall -Wno-missing-braces
CFDEBUG  = -g -DDEBUG
SDIR = src
ODIR = obj
IDIR = include
BDIR = bin
SRCFILES := $(wildcard $(SDIR)/*.c)
OBJFILES := $(subst $(SDIR), $(ODIR), $(SRCFILES:%.c=%.o))

PROGRAM = assembler

all: $(BDIR)/$(PROGRAM)

.PHONY: clean

debug: CFLAGS += $(CFDEBUG)
debug: $(BDIR)/$(PROGRAM)

$(BDIR)/$(PROGRAM): $(OBJFILES)
	@mkdir -p $(BDIR)
	$(CC) $(CFLAGS) -I$(IDIR) $(OBJFILES) -o $(BDIR)/$(PROGRAM)
	@echo "\nSuccessfuly built program '$(BDIR)/$(PROGRAM)'"

$(ODIR)/%.o: $(SDIR)/%.c
	@mkdir -p $(ODIR)
	$(CC) $(CFLAGS) -I$(IDIR) -c $< -o $@

clean: 
	rm -rf $(ODIR) $(BDIR)