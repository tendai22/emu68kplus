#! /bin/sh
b=`basename -s .s "$1"`
m68k-elf-as -o ${b}.o "$1" && m68k-elf-objdump -D ${b}.o
