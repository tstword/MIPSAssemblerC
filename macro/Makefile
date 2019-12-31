# Compiler, initially GCC however can be changed if needed
CC       = gcc
CFLAGS   = -Wall -Wextra -O2
CFDEBUG  = -g -DDEBUG
SDIR = ../src
ODIR = obj
IDIR = ../include
BDIR = bin
SRCFILES := $(shell find $(SDIR) -type f -name "*.c" ! -name "main.c")
OBJFILES := $(subst $(SDIR), $(ODIR), $(SRCFILES:%.c=%.o))
PROGRAM = macro

all: $(BDIR)/$(PROGRAM)

.PHONY: clean

debug: CFLAGS += $(CFDEBUG)
debug: $(BDIR)/$(PROGRAM)

$(BDIR)/$(PROGRAM): $(OBJFILES) macro.c
	@mkdir -p $(BDIR)
	$(CC) $(CFLAGS) -I$(IDIR) $(OBJFILES) macro.c -o $(BDIR)/$(PROGRAM)
	@echo "\nSuccessfuly built program '$(BDIR)/$(PROGRAM)'"

$(ODIR)/%.o: $(SDIR)/%.c
	@mkdir -p $(ODIR)
	$(CC) $(CFLAGS) -I$(IDIR) -c $< -o $@

clean: 
	rm -rf $(ODIR) $(BDIR)
