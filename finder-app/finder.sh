#!/bin/sh
# Assignment 1 finder-app finder.sh 
# Author: Sonal Tamrakar
# Date: 08/31/2024


if [ $# -ne 2 ] || [ ! -d "$1" ] || [ -d "$2" ]
then
    echo "ERROR: Invalid Number of Arguments."
    echo "Total number of arguments should be 2."
    echo "The order of the arguments should be:"
    echo "1) File Directory Path."
    echo "2) String to be searched in the specified directory path."
    exit 1
fi


FILESDIR=$1
SEARCHSTR=$2


MATCHES=$(grep -ro $SEARCHSTR $FILESDIR | wc -l)
FILESCNT=$(find $FILESDIR -type f | wc -l)

echo The number of files are ${FILESCNT} and the number of matching lines are ${MATCHES}
exit 0

