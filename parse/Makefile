all: cueparser cuelexer

lex.cue.c: cue.L cue.tab.h
	flex -Pcue cue.L

lex.cuelex.c: cue.L cue.tab.h
	flex -Pcuelex cue.L

lex.cue.o: lex.cue.c
	gcc -g -Wall -c lex.cue.c

lex.cuelex.o: lex.cuelex.c
	gcc -g -DSTANDALONE -Wall -c lex.cuelex.c

cue.tab.c: cue.y
	bison -p cue -d cue.y

cue.tab.o: cue.tab.c cue.tab.h
	gcc -g -Wall -DSTANDALONE -c cue.tab.c

cueparser: lex.cue.o cue.tab.o
	gcc -g lex.cue.o cue.tab.o -lfl -o cueparser

cuelexer: lex.cuelex.o 
	gcc -g lex.cuelex.o -lfl -o cuelexer
