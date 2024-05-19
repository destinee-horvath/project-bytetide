#!/bin/bash

cd tests

#to run all the tests in test folder 
for i in {1..10}
do
    # gcc src/pkgmain.c src/chk/pkgchk.c src/crypt/sha256.c src/tree/merkletree.c -Iinclude -Wall -std=c2x -g -fsanitize=address -lm -lpthread -o tests/test$i/pkgmain
    cd "test$i"
    chmod +x run_test.sh
    ./run_test.sh
    cd ..
    echo ""

done

for i in {1..10}
do
    rm -f pkgmain
done
 