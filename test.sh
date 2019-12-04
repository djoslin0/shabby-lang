#!/bin/bash

echo "Building..."
./build.sh > /dev/null
echo "  Done."
echo ""
echo "Testing..."
set -e

run_test(){
	local src_file=`realpath $1`
	echo "  $src_file"
	cd bin
	./tokenizer $src_file > /dev/null
	./parser $src_file > /dev/null
	./symgen $src_file > /dev/null
	./typec $src_file > /dev/null
	./codegen $src_file > /dev/null
	./jumpr $src_file > /dev/null
	./vm $src_file > /dev/null
	cd ..
}

for file in tests/pass/*
do
  run_test $file
done
echo ""
echo "Passed!"