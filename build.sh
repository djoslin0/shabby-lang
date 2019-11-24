#!/bin/bash

cd src

rm ../bin/*

echo "#############"
echo "# Tokenizer #"
echo "#############"
gcc tokenizer.c symbols.c file.c -I include -o "../bin/tokenizer" -Wall -Werror -Wpedantic
../bin/tokenizer

echo ""
echo "##########"
echo "# Parser #"
echo "##########"
gcc parser.c symbols.c file.c -I include -o "../bin/parser" -Wall -Werror -Wpedantic
../bin/parser

echo ""
echo "###########"
echo "# Codegen #"
echo "###########"
gcc codegen.c symbols.c file.c -I include -o "../bin/codegen" -Wall -Werror -Wpedantic
../bin/codegen
