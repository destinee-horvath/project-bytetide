#!/bin/bash

date

echo "Test 10"

in_file="merk_test10.in"
touch "merk_test10.expected"
expected_file="merk_test10.expected"
out_file="merk_test10.out"

./pkgmain test10.bpkg -test_data corrupt.data > merk_test10.out

if diff "$out_file" "$expected_file" >/dev/null; then
    echo "Passed Test 10"
else
    echo "Failed Test 10"
    echo "-- Expected Output --"
    cat "$expected_file"
    echo ""
    echo "-- Actual Output --" 
    cat "$out_file"
    echo ""
fi

