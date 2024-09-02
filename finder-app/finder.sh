#!/bin/sh
# Assignment 1 finder-app finder.sh 
# Author: Sonal Tamrakar
# Date: 08/31/2024

# error checking, ensuring $1 is a directory and $2 not a directory
# make sure there are only two arguments passed
if [ $# -ne 2 ] || [ ! -d "$1" ] || [ -d "$2" ]
then
    echo "ERROR: Arguments != 2 and/or"
    echo "ERROR: Directory could not be found and/or"
    echo "ERROR: 2nd argument is a directory, and not a string."
    echo "Expected order of the arguments should be:"
    echo "1) Directory Path FILESDIR to search for SEARCHSTR"
    echo "2) string SEARCHSTR"
    exit 1
fi


FILESDIR=$1
SEARCHSTR=$2


MATCHES=$(grep -ro $SEARCHSTR $FILESDIR | wc -l)
FILESCNT=$(find $FILESDIR -type f | wc -l)

echo The number of files are ${FILESCNT} and the number of matching lines are ${MATCHES}


