#!/bin/bash

date

echo "Test 2"

touch "merk_test2.expected"
expected_file="merk_test2.expected"
out_file="merk_test2.out"

./pkgmain test2.bpkg -test_bpkg test2.data > merk_test2.out

if diff "$out_file" "$expected_file" >/dev/null; then
    echo "Passed Test 2"
else
    echo "Failed Test 2"
    echo "-- Expected Output --"
    cat "$expected_file"
    echo ""
    echo "-- Actual Output --" 
    cat "$out_file"
    echo ""
fi
