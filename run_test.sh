#!/bin/bash
make clean
sleep 1
clear
make &> compile_out.txt
sleep 1
clear
echo -e "clang -Wall -g -o  main main.c libmythreads.a\n"
clang-3.6 -Wall -g -o  main main.c libmythreads.a
./main
grep --color=always -rn "error" compile_out.txt
