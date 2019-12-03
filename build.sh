#!/bin/bash

src_file=`realpath $1`

cd src

kill `pidof display`
rm -rf ../bin/*
mkdir ../bin/compilation/

set -e

echo "#############"
echo "# Tokenizer #"
echo "#############"
gcc tokenizer.c utils/symbols.c utils/file.c -I include -o "../bin/tokenizer" -Wall -Wextra -Werror -Wpedantic
../bin/tokenizer $src_file

echo ""
echo "##########"
echo "# Parser #"
echo "##########"
gcc parser.c utils/symbols.c utils/file.c utils/nodes.c -I include -o "../bin/parser" -Wall -Wextra -Werror -Wpedantic
../bin/parser $src_file

echo ""
echo "##########"
echo "# Symgen #"
echo "##########"
gcc symgen.c utils/symbols.c utils/file.c utils/nodes.c utils/types.c utils/variables.c -I include -o "../bin/symgen" -Wall -Wextra -Werror -Wpedantic
../bin/symgen $src_file

echo ""
echo "###############"
echo "# Typechecker #"
echo "###############"
gcc typechecker.c utils/symbols.c utils/file.c utils/nodes.c utils/types.c utils/variables.c -I include -o "../bin/typec" -Wall -Wextra -Werror -Wpedantic
../bin/typec $src_file


# only generates graph if graphviz is installed
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

echo ""
echo "###########"
echo "# Codegen #"
echo "###########"
gcc codegen.c utils/symbols.c utils/file.c utils/nodes.c utils/types.c utils/variables.c -I include -o "../bin/codegen" -Wall -Wextra -Werror -Wpedantic
../bin/codegen $src_file

echo ""
echo "#################"
echo "# Jump Resolver #"
echo "#################"
gcc jumpresolver.c utils/file.c -I include -o "../bin/jumpr" -Wall -Wextra -Werror -Wpedantic
../bin/jumpr $src_file


echo ""
echo "######"
echo "# VM #"
echo "######"
gcc vm.c utils/symbols.c utils/file.c -I include -o "../bin/vm" -Wall -Wextra -Werror -Wpedantic
../bin/vm $src_file

