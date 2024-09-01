#!/bin/sh
# Assignment 1 finder-app writer.sh 
# Author: Sonal Tamrakar
# Date: 08/31/2024

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

if [ ! -d "$WRITEFILEDIR" ]
then
   mkdir -p "$WRITEFILEDIR"
fi

if [ ! -f "$WRITEFILE" ]
then
   echo "$WRITESTR" > "$WRITEFILE"
fi