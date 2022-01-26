#!/bin/sh

cd scan

for f in ../*C200*7z; do
  echo "Firmware $f" >> ../dump
  echo "Deal with $f"
  7zr e $f
  echo "Scan these:" >> ../dump
  ls *rom >> ../dump
  strings -t x *.rom | grep "Sansa C200" >> ../dump
  # now clear all files
  rm *
done
