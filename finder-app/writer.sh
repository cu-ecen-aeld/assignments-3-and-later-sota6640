#!/bin/sh
# Assignment 1/2 finder-app writer.sh 
# Author: Sonal Tamrakar
# Date: 08/31/2024

# error checking
if [ $# -ne 2 ] || [ -d "$2" ]
then
    echo "ERROR: Arguments != 2 and/or"
    echo "Expected order of the arguments should be:"
    echo "1) File Directory Path Including Filename."
    echo "2) String WRITESTR to be written in the specified directory file."
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
# write to file. if file doesn't exist, makes the file and then writes to it
echo "$WRITESTR" > "$WRITEFILE"

if [ $? -eq 1 ]
then
   echo The file could not be created.
   exit 1
else
   exit 0
fi