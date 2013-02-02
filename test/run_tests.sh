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


echo "ran $(($passing+$failing)) tests"
echo "$passing passed"
echo "$failing failed"




