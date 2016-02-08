#!/usr/bin/env bash

#
# Enforce EOF new line
#

ENFORCED_FILES="LeapSerial src"

# Go to root directory
for f in $(find $ENFORCED_FILES -name *.hpp -o -name *.cpp -o -name *.h -o -name CMakeLists.txt);
do
  if [ "$(tail -c 1 $f)" != "" ];
  then
    if [ "$BAD_FILES" == "" ]
    then
      echo
      echo "Some files are missing trailing newlines"
      echo
      BAD_FILES=true
    fi

    echo $f
  fi
done

if [ $BAD_FILES ]; then
  echo
  exit 1
fi
