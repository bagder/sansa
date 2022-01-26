#!/bin/sh
for i in `cat diffbl`; do
  #echo "crc $i"
  ./e200crc $i
done
