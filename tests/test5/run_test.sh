#!/bin/bash

date

echo "Test 5"

in_file="merk_test5.in"
touch "merk_test5.expected"
expected_file="merk_test5.expected"
out_file="merk_test5.out"

./pkgmain test5.bpkg -test_bpkg test5.data > merk_test5.out

if diff "$out_file" "$expected_file" >/dev/null; then
    echo "Passed Test 5"
else
    echo "Failed Test 5"
    echo "check .expected and .out file"
    echo "too large to display"
    
    # echo "-- Expected Output --"
    # cat "$expected_file"
    # echo ""
    # echo "-- Actual Output --" 
    # cat "$out_file"
    # echo ""
fi

