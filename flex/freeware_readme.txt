FLEX, UTILITIES, fast lexical analyzer generator for OpenVMS

GNU flex v2.5.4 for OpenVMS VAX and OpenVMS Alpha

flex is a tool for generating scanners: programs which recognized
lexical patterns in text.flex reads the given input files, or its
standard input if no file names are given, for a description of a
scanner to generate.  The description is in the form of pairs of
regular expressions and C code, called rules. flex generates as output
a C source file, lex.yy.c, which defines a routine yylex(). This file
is compiled and linked with the -lfl library to produce an executable.
When the executable is run, it analyzes its input for occurrences of
the regular expressions.Whenever it finds one, it executes the
corresponding C code.

VMS binaries supplied by Hunter Goatley
