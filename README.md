C Struct Member Offset
----------------------

A tool for printing the offset of a member in a C struct given the struct and
member name on the commandline, and some C containing the structure definition
on stdin.

```
$ ./csmo
Usage: ./csmo <STRUCT NAME> <MEMBER NAME>

  Struct definition required on stdin

Example:

  $ ./csmo a d <<< "struct a {int b; char c; int d; };"
  a.d: 8
```
