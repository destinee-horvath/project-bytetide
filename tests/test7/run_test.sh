#!/bin/bash

date

echo "Test 7"

in_file="merk_test7.in"
touch "merk_test7.expected"
expected_file="merk_test7.expected"
out_file="merk_test7.out"

./pkgmain test7.bpkg -test_tree test7.data > merk_test7.out

while IFS= read -r line
do
    if ! grep -Fxq "$line" "$expected_file"; then   
        echo "'$line' does not exist in .expected file."
        count=$((count + 1))
    fi
done < "$out_file" 
echo "Number hashes not found in test6.expected: $missing_count"
