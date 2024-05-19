#!/bin/bash

date

echo "Test 3"

in_file="merk_test3.in"
touch "merk_test3.expected"
expected_file="merk_test3.expected"
out_file="merk_test3.out"

./pkgmain test3.bpkg -test_bpkg test3.data > merk_test3.out

if diff "$out_file" "$expected_file" >/dev/null; then
    echo "Passed Test 3"
else
    echo "Failed Test 3"
    echo "-- Expected Output --"
    cat "$expected_file"
    echo ""
    echo "-- Actual Output --" 
    cat "$out_file"
    echo ""
fi

