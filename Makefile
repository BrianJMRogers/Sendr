PROG_NAME=sendr
PATHTOPROG=src

BINDIR=./bin

COMPILE_COMMAND=gcc

$(PROG_NAME):
	@if [ ! -d $(BINDIR) ] ; \
	then                     \
		mkdir $(BINDIR);       \
	fi

	$(COMPILE_COMMAND) $(PATHTOPROG)/$(PROG_NAME).c -o $(BINDIR)/$(PROG_NAME) 

clean CLEAN:
	rm -r $(BINDIR)
