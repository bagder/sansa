#!/bin/sh

cd scan

for f in ../*E200*7z; do
  ver=`echo $f | sed -e 's/\.7z//g' -e 's/..\/SansaE200//g'`
  echo "Deal with $f => $ver"
  7zr e $f
  mkdir -p rom/$ver
  cp *.rom rom/$ver/
  # now clear all files
  rm *
done
