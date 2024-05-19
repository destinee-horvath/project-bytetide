#!/bin/bash

date

echo "Test 9"

in_file="merk_test9.in"
touch "merk_test9.expected"
expected_file="merk_test9.expected"
out_file="merk_test9.out"

./pkgmain test9.bpkg -all_hashes test9.data > merk_test9.out

if diff "$out_file" "$expected_file" >/dev/null; then
    echo "Passed Test 9"
else
    echo "Failed Test 9"
    echo "-- Expected Output --"
    cat "$expected_file"
    echo ""
    echo "-- Actual Output --" 
    cat "$out_file"
    echo ""
fi
