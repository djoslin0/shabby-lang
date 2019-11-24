#!/bin/bash

echo "#############"
echo "# Tokenizer #"
echo "#############"
gcc tokenizer.c symbols.c symbols.h file.c file.h constants.h -o ./out/tokenizer -Wall -Werror -Wpedantic
./out/tokenizer

echo ""
echo "##########"
echo "# Parser #"
echo "##########"
gcc parser.c symbols.c symbols.h file.c file.h constants.h -o ./out/parser -Wall -Werror -Wpedantic
./out/parser
