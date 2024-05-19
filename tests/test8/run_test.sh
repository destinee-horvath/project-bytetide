#!/bin/bash

date

echo "Test 8"

in_file="merk_test8.in"
touch "merk_test8.expected"
expected_file="merk_test8.expected"
out_file="merk_test8.out"

./pkgmain test8.bpkg -test_data test8.data > merk_test8.out

if diff "$out_file" "$expected_file" >/dev/null; then
    echo "Passed Test 8"
else
    echo "Failed Test 8"
    echo "-- Expected Output --"
    cat "$expected_file"
    echo ""
    echo "-- Actual Output --" 
    cat "$out_file"
    echo ""
fi
