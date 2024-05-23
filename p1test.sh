#!/bin/bash

cd tests

#to run all the tests in test folder 
for i in {1..10}
do
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
 