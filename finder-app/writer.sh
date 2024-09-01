#!/bin/sh
# Assignment 1 finder-app writer.sh 
# Author: Sonal Tamrakar
# Date: 08/31/2024

# error checking
if [ $# -ne 2 ] || [ -d "$2" ]
then
    echo "ERROR: Invalid Number of Arguments."
    echo "Total number of arguments should be 2."
    echo "The order of the arguments should be:"
    echo "1) File Directory Path Including Filename."
    echo "2) String to be written in the specified directory file."
    exit 1
fi

WRITEFILE=$1
WRITESTR=$2

WRITEFILEDIR="$(dirname $WRITEFILE)"

# if directory doesn't exist, create it
if [ ! -d "$WRITEFILEDIR" ]
then
   mkdir -p "$WRITEFILEDIR"
fi

echo "$WRITESTR" > "$WRITEFILE"

if [ $? -eq 1 ]
then
   echo The file could not be created.
   exit 1
else
   exit 0
fi