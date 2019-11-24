#!/bin/bash

echo "#############"
echo "# Tokenizer #"
echo "#############"
gcc tokenizer.c symbols.c file.c -o ./out/tokenizer -Wall -Werror -Wpedantic
./out/tokenizer

echo ""
echo "##########"
echo "# Parser #"
echo "##########"
gcc parser.c symbols.c file.c -o ./out/parser -Wall -Werror -Wpedantic
./out/parser

echo ""
echo "###########"
echo "# Codegen #"
echo "###########"
gcc codegen.c symbols.c file.c -o ./out/codegen -Wall -Werror -Wpedantic
./out/codegen
