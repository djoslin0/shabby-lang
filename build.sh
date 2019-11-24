#!/bin/bash

cd src

rm ../bin/*

echo "#############"
echo "# Tokenizer #"
echo "#############"
gcc tokenizer.c symbols.c file.c -I include -o "../bin/tokenizer" -Wall -Werror -Wpedantic
../bin/tokenizer $1

echo ""
echo "##########"
echo "# Parser #"
echo "##########"
gcc parser.c symbols.c file.c -I include -o "../bin/parser" -Wall -Werror -Wpedantic
../bin/parser $1

echo ""
echo "###########"
echo "# Codegen #"
echo "###########"
gcc codegen.c symbols.c file.c -I include -o "../bin/codegen" -Wall -Werror -Wpedantic
../bin/codegen $1

echo ""
echo "######"
echo "# VM #"
echo "######"
gcc vm.c symbols.c file.c -I include -o "../bin/vm" -Wall -Werror -Wpedantic
../bin/vm $1
