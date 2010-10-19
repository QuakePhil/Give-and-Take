#!/bin/sh
# gcc -O9 -funroll-loops -march=native -mtune=native -m64 -m3dnow -msse -msse2 -msse3 gnt.c -o gnt
gcc -O9 -funroll-loops -march=native -mtune=native -m3dnow -msse -msse2 -msse3 gnt.c -o gntdev
