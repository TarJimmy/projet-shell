#!/bin/bash

if [ -f "shell" ]
then 
    ./sdriver.pl -t $1 -s ./shell
else
    echo "Shell not exist"
fi
