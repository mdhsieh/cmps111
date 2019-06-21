all: argshell

argshell: lex.yy.c
	cc -o argshell argshell.c lex.yy.c -lfl

lex.yy.c: shell.l
	flex shell.l

clean:
	rm -f argshell lex.yy.c
