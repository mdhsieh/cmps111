Shell program in C.

Modified: argshell.c, designdoc, Makefile, README

---

Michael Hsieh

To compile:
run make, equivalent to
flex shell.l
cc -o argshell argshell.c lex.yy.c -lfl

To run:
./argshell

Details:
The shell works for single commands, with or
without flags, using <, >, >>, or |.

After some limited tests, non-piped multiple
commands separated by semicolons should also work.

Redirection of standard error and chaining
multiple commands via pipes were not implemented.

Files:

Makefile

README

argshell.c

designdoc.pdf

shell.l
