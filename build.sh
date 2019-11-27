#!/bin/bash

cd src

rm ../bin/*

set -e

echo "#############"
echo "# Tokenizer #"
echo "#############"
gcc tokenizer.c utils/symbols.c utils/file.c -I include -o "../bin/tokenizer" -Wall -Werror -Wpedantic
../bin/tokenizer $1

echo ""
echo "##########"
echo "# Parser #"
echo "##########"
gcc parser.c utils/symbols.c utils/file.c utils/nodes.c -I include -o "../bin/parser" -Wall -Werror -Wpedantic
../bin/parser $1

echo ""
echo "###############"
echo "# Typechecker #"
echo "###############"
gcc typechecker.c utils/symbols.c utils/file.c utils/nodes.c utils/types.c utils/variables.c -I include -o "../bin/typec" -Wall -Werror -Wpedantic
../bin/typec $1

# only generates graph if graphviz is installed
if hash dot 2>/dev/null; then
  echo ""
  echo "#########"
  echo "# Graph #"
  echo "#########"
  gcc graphviz.c utils/symbols.c utils/file.c utils/nodes.c -I include -o "../bin/graph" -Wall -Werror -Wpedantic
  ../bin/graph $1
  dot -Tpng ../bin/$1.dot > ../bin/$1.png
fi

if hash display 2>/dev/null; then
  display ../bin/$1.png &
fi

echo ""
echo "###########"
echo "# Codegen #"
echo "###########"
gcc codegen.c utils/symbols.c utils/file.c utils/nodes.c utils/types.c utils/variables.c -I include -o "../bin/codegen" -Wall -Werror -Wpedantic
../bin/codegen $1

echo ""
echo "######"
echo "# VM #"
echo "######"
gcc vm.c utils/symbols.c utils/file.c -I include -o "../bin/vm" -Wall -Werror -Wpedantic
../bin/vm $1

