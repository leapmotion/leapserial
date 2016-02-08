#!/usr/bin/env bash

#
# Enforce spaces instead of tabs
#

ENFORCED_FILES="LeapSerial src CMakeLists.txt"

# Go to root directory

if grep -qr $'\t' $ENFORCED_FILES
then
  echo
  echo "The LeapSerial project prohibits tabs for spacing."
  echo
  echo "Tabs found in the following files:"
  grep -rn $'\t' $ENFORCED_FILES
  exit 1
fi
