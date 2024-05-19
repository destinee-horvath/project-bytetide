#!/bin/bash

date

echo "Test 6"

touch "merk_test6.expected"
expected_file="merk_test6.expected"
out_file="merk_test6.out"

count=0

./pkgmain test6.bpkg -test_tree test6.data > merk_test6.out

while IFS= read -r line
do
    if ! grep -Fxq "$line" "$expected_file"; then           #if line in out_file doesnt exist in expected_file
        echo "'$line' does not exist in .expected file."
        count=$((count + 1))
    fi
done < "$out_file" #iterate through out_file

echo "Number hashes not found in test6.expected: $missing_count"

