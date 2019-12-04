#!/bin/bash

if [ "$#" -eq 1 ]; then src_file=`realpath $1`; fi

cd src

kill `pidof display` 2> /dev/null
rm -rf ../bin/*
mkdir ../bin/compilation/

set -e

echo "#############"
echo "# Tokenizer #"
echo "#############"
gcc tokenizer.c utils/symbols.c utils/file.c -I include -o "../bin/tokenizer" -Wall -Wextra -Werror -Wpedantic
if [ "$#" -eq 1 ]; then ../bin/tokenizer $src_file; fi

echo ""
echo "##########"
echo "# Parser #"
echo "##########"
gcc parser.c utils/symbols.c utils/file.c utils/nodes.c -I include -o "../bin/parser" -Wall -Wextra -Werror -Wpedantic
if [ "$#" -eq 1 ]; then ../bin/parser $src_file; fi

echo ""
echo "##########"
echo "# Symgen #"
echo "##########"
gcc symgen.c utils/symbols.c utils/file.c utils/nodes.c utils/types.c utils/variables.c -I include -o "../bin/symgen" -Wall -Wextra -Werror -Wpedantic
if [ "$#" -eq 1 ]; then ../bin/symgen $src_file; fi

echo ""
echo "###############"
echo "# Typechecker #"
echo "###############"
gcc typechecker.c utils/symbols.c utils/file.c utils/nodes.c utils/types.c utils/variables.c -I include -o "../bin/typec" -Wall -Wextra -Werror -Wpedantic
if [ "$#" -eq 1 ]; then ../bin/typec $src_file; fi


# only generates graph if graphviz is installed
if [ "$#" -eq 1 ]; then
  if hash dot 2>/dev/null; then
    echo ""
    echo "#########"
    echo "# Graph #"
    echo "#########"
    gcc graphviz.c utils/symbols.c utils/file.c utils/nodes.c -I include -o "../bin/graph" -Wall -Wextra -Werror -Wpedantic
    ../bin/graph $src_file
    dot -Tpng ../bin/compilation/out.dot > ../bin/compilation/out.png
  fi

  if hash display 2>/dev/null; then
    display ../bin/compilation/out.png &
  fi
fi

echo ""
echo "###########"
echo "# Codegen #"
echo "###########"
gcc codegen.c utils/symbols.c utils/file.c utils/nodes.c utils/types.c utils/variables.c -I include -o "../bin/codegen" -Wall -Wextra -Werror -Wpedantic
if [ "$#" -eq 1 ]; then ../bin/codegen $src_file; fi

echo ""
echo "#################"
echo "# Jump Resolver #"
echo "#################"
gcc jumpresolver.c utils/file.c -I include -o "../bin/jumpr" -Wall -Wextra -Werror -Wpedantic
if [ "$#" -eq 1 ]; then ../bin/jumpr $src_file; fi


echo ""
echo "######"
echo "# VM #"
echo "######"
gcc vm.c utils/symbols.c utils/file.c -I include -o "../bin/vm" -Wall -Wextra -Werror -Wpedantic
if [ "$#" -eq 1 ]; then ../bin/vm $src_file; fi

