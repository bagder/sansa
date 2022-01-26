#!/bin/sh

cd scan

for f in ../*7z; do
  echo "Firmware $f" >> ../dump
  echo "Deal with $f"
  7zr e $f
  echo "Scan these:" >> ../dump
  ls *mi4 *rom >> ../dump
  mi4code keyscan *.mi4 *.rom >> ../dump
  # now clear all files
  rm *
done
