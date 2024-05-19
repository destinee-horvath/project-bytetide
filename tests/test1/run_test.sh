#!/bin/bash

date

echo "Test 1"

touch "merk_test1.expected"
expected_file="merk_test1.expected"
out_file="merk_test1.out"

./pkgmain test1.bpkg -test_bpkg test1.data > merk_test1.out

if diff "$out_file" "$expected_file" >/dev/null; then
    echo "Passed Test 1"
else
    echo "Failed Test 1"
    echo "-- Expected Output --"
    cat "$expected_file"
    echo ""
    echo "-- Actual Output --" 
    cat "$out_file"
    echo ""
fi

