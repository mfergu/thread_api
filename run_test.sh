#!/bin/bash
make clean
rm main
clear
make &> compile_out.txt
sleep 1
clear
echo -e "clang -Wall -g -o  main main.c libmythreads.a\n"
clang -Wall -g -o  main main.c libmythreads.a
./main
grep --color=always -rn "error" compile_out.txt
