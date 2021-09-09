#!/bin/bash

tests=$(ls tests/*.in)

for i in $tests; do
	o="${i%in}out"
	t="${i%in}test"
	./main < "$i" > "$t"
	diff "$t" "$o"
	if [ $? -ne 0 ] ; then exit 1 ; fi
	rm "$t"
done

echo 'All tests passed'


