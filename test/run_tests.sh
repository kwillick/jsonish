#!/bin/sh

#passing top level objects
passing=0
failing=0
for pass_file in pass/*object.json; do
    if ! ./tester $pass_file pass object; then
        ((failing=$failing+1))
    else
        ((passing=$passing+1))
    fi
done

for pass_file in pass/*array.json; do
    if ! ./tester $pass_file pass array; then
        ((failing=$failing+1))
    else
        ((passing=$passing+1))
    fi
done

for fail_file in fail/*.json; do
    if ! ./tester $fail_file fail; then
        ((failing=$failing+1))
    else
        ((passing=$passing+1))
    fi
done

echo "ran $(($passing+$failing)) tests"
echo "\033[34m$passing passed\033[0m"
echo "\033[31m$failing failed\033[0m"




