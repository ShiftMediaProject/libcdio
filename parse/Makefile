INCLUDES = -I .
all: cueparser cuelexer tocparser

lex.cue.c: cue.L cue.tab.h
	flex -Pcue cue.L

lex.cuelex.c: cue.L cue.tab.h
	flex -Pcuelex cue.L

lex.cue.o: lex.cue.c
	gcc -g -Wall -c lex.cue.c $(INCLUDES)

lex.cuelex.o: lex.cuelex.c
	gcc -g -DSTANDALONE -Wall -c lex.cuelex.c

cue.tab.c, cue.tab.h: cue.y
	bison -p cue -d cue.y

cue.tab.o: cue.tab.c cue.tab.h
	gcc -g -Wall -DSTANDALONE -c cue.tab.c $(INCLUDES)

cueparser: lex.cue.o cue.tab.o
	gcc -g lex.cue.o cue.tab.o -lfl -o cueparser

cuelexer: lex.cuelex.o 
	gcc -g lex.cuelex.o -lfl -o cuelexer

toc.tab.h: toc.tab.c

toc.tab.c: toc.y
	bison -p toc -d toc.y

toclexer.o: toclexer.c
	gcc -g -Wall -c toclexer.c $(INCLUDES)

toc.tab.o: toc.tab.c toc.tab.h
	gcc -g -Wall -DSTANDALONE -c toc.tab.c $(INCLUDES)

tocparser: toc.tab.o toclexer.o
	gcc -g toclexer.o toc.tab.o -o tocparser

clean: 
	rm -f lex.cue.c lex.cuelex.c lex.cue.o lex.cuelex.o cue.tab.c \
	    cue.tab.o cueparser cuelexer

check:
	(cd test && ./runall)
