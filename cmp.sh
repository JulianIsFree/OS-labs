#!/bin/bash

if [ -n "$1" ]
then
echo cc oslab$1.c -o l$1.out -mt -lpthread
cc oslab$1.c -o l$1.out -mt -lpthread
else
echo "Enter lab number"
fi
