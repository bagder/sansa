#!/bin/sh

rm dump

for f in e200/PP5022.mi4 e200/SKU_E-PP5022.mi4 h10/H10.mi4 medion/JUKEBOX.mi4 philips/philips-FWImage.ebn virgin/PP5020.MI4 yh925gs/20050324100034734_FW_YH925.mi4 tatung/pp5020.mi4 m-audio/PP5020.mi4 aldipod/JUKEBOX.mi4 sirrius/S50main.mi4 yh820/FW_YH820.mi4 yh920/PP5020.mi4 yh925/20050324095916500_FW_YH925.mi4; do
  #echo "$f"
  md5sum $f >> dump
done
