#!/bin/bash

date

echo "Test 4"

in_file="merk_test4.in"
touch "merk_test4.expected"
expected_file="merk_test4.expected"
out_file="merk_test4.out"

./pkgmain test4.bpkg -test_bpkg test4.data > merk_test4.out

if diff "$out_file" "$expected_file" >/dev/null; then
    echo "Passed Test 4"
else
    echo "Failed Test 4"
    echo "-- Expected Output --"
    cat "$expected_file"
    echo ""
    echo "-- Actual Output --" 
    cat "$out_file"
    echo ""
fi