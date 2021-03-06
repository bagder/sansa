#include "doctype.t"
#include "setup.t"
#include "daniel.t"
HEAD(The mi4 File Format)
#include "body.t"
#include "topmenu.t"

TITLE(mi4)
BOXTOP
<p> The mi4 file format is something PortalPlayer provides or supports, as
 numerous PortalPlayer based music players use that for firmware upgrades.

<p>
 All glory to <a href="mrh.html">MrH</a> for his hard digging on this.

BOXBOT

TITLE(mi4code)
BOXTOP

<p> <a href="mi4code.html">mi4code</a> is a tool for dealing with mi4 and BL
files.

BOXBOT

TITLE(mi4 collection)
BOXTOP
<p>

#define FINE <td> OK </td>
#define FAIL <td> FAIL </td>
#define NOTTRIED <td> ? </td>
#define MD5(x) <td style="font-size:70%;"> x </td>

An <a href="/sansa/fw/">extensive collection of most (if not all) existing
firmwares for Sansa E200 an Sansa C200</a>

<table>

<tr><th>mi4 file</th><th>decode</th><th>thanks to</th><th>BL file</th><th>Notes</th><th>mi4 md5sum</th></tr>

<tr><td> <a href="sansa-view/Sansa%20View%2001.00.03/firmware.mi4">SanDisk Sansa View</a></td> FINE
<td>zivan56</td><td><a href="sansa-view/Sansa%20View%2001.00.03/bootLoader.rom">BL rom</a></td><td>1.00.03</td>MD5(27124dcbe7425cf36314ae9d7454826a) </tr>

<tr><td> <a href="c200/firmware.mi4">SanDisk Sansa c200</a></td> FINE
<td>zefie</td><td><a href="c200/pribootLoader.rom">BL rom</a></td><td>1.00.06</td>MD5(7d4de4ac02f6c3d8aeff88c8872b08a5) </tr>

<tr><td> <a href="e200/PP5022.mi4">SanDisk Sansa e200 (American)</a></td> FINE
<td></td><td><a href="e200/BL_SD_boardSupportSD.rom">BL
rom</a></td><td>1.00.12</td> MD5(2c11dc0b10f1393115c54b17b6278cd2) </tr>

<tr><td> <a href="e200/1.01.11A/PP5022.mi4">SanDisk Sansa e200
(American)</a></td> FINE <td>lgianasi</td><td><a
href="e200/1.01.11A/BL_SD_boardSupportSD.rom">BL
rom</a></td><td>1.01.11A</td> MD5(8fbdfa44671e936bb2fffc9e11e8bedb) </tr>

<tr><td> <a href="e200/1.02.15a.mi4">SanDisk Sansa e200 (American)</a></td>
FINE <td>matze</td><td>regular BL</td><td>1.02.15a</td>
MD5(e83a74b44accc88f64f1e81039200442) </tr>

<tr><td> <a href="e200/SKU_E-PP5022.mi4">SanDisk Sansa e200
(European)</a></td> FINE <td>Dave Chapman</td><td><a
href="e200/BL_SD_boardSupportSD.rom">BL rom</a></td><td>1.00.12</td>
MD5(86db01ed5175f2e18ff833baa9f2660c) </tr>

<tr><td> <a href="e200/rhapsody/rhapsody-1.0.2.31.zip">SanDisk Sansa e250R
</a></td> FINE <td>Kpt Kill & MrH</td><td><a
href="e200/BL_SD_boardSupportSD.btl">BL btl</a></td><td>1.0.2.31</td>
MD5(c9d7b92492ceddf8c32008e925aee5f4) </tr>

<tr><td> <a href="h10/H10.mi4">iriver H10</a></td> FINE <td></td><td><a
href="h10/BL_H10.rom">BL rom</a></td><td></td> MD5(d4f5cc4e52124249cd6880ae0c1cbae5)
</tr>

<tr><td> <a href="h10-20GC/H10_20GC.mi4">iriver H10 20GC/MTP</a></td> FINE
<td>Barry Wardell</td><td><a href="h10-20GC/BL_H10_20GC.hex">BL hex</a></td>
<td></td> MD5(c57b2cf2a6b49dbbc43645fecee6a576) </tr>

<tr><td> <a href="medion/JUKEBOX.mi4">Medion MD 95x00</a></td> FINE <td></td>
<td></td><td></td>MD5(8562a83695383a5f09290225d89ca68d) </tr>

<tr><td> <a href="philips/philips-FWImage.ebn">Philips HDD6330</a></td> FAIL
<td></td><td></td><td>needs BL to decode</td> MD5(a58edfeb2745ac132119b3c9b5742a12) </tr>

<tr><td> <a href="philips-hdd120_00/Philips.mi4">Philips HDD120/00</a></td>
FINE <td>Marcoen Hirschberg</td><td></td><td></td>
MD5(a0393696971474cc73328426e482bf9e) </tr>

<tr><td> <a href="sa9200/FWImage.ebn">Philips SA9200</a></td>
FINE <td><a href="http://www.p4c.philips.com/files/s/">philips.com</a></td><td><a href="sa9200/sa9200-bl.zip">BL</a></td><td>BL ripped from target</td>
MD5(d384fe46e2e40ed80fef6a45ed1e2944) </tr>

<tr><td> <a href="virgin/PP5020.MI4">Virgin Electronics</a></td> FINE
<td></td><td><a href="virgin/bootloader.rom">BL rom</a></td> <td></td>
MD5(318df5fecc7327fdc14620dc9885baf1) </tr>

<tr><td> <a href="tatung/pp5020.mi4">Tatung Elio M310</a></td> FINE <td>Jason
Simpson</td> <td></td><td></td>MD5(e5e0c4f1d8f5a5e0db0494a9246966d0) </tr>

<tr><td> <a href="tatung/P722/PP5020-L.mi4">Tatung Elio P722</a> <a
href="tatung/P722/PP5020-U.mi4">#2</a></td> FINE <td></td><td></td> <td></td>
MD5(f96200548d9923042345814d0fad0af9) </tr>

<tr><td> <a href="tatung/P810/PP5020.mi4">Tatung Elio P810</a></td> FAIL
<td></td><td></td><td>needs BL to decode</td>
MD5(589641eff7c0ed70c3aaa96922bd3834) </tr>

<tr><td> <a href="tatung/TPJ1022/pp5020-elio-tpj1022.mi4">Tatung Elio
TPJ-1022</a></td> FINE <td>Dave Chapman</td><td></td><td>010201 using default
key</td> MD5(2be6b2878ef8fd385aa8a52cbccefcb6) </tr>

<tr><td> <a href="m-audio/PP5020.mi4">M-Audio MicroTrack 24/96</a></td> FINE
<td>Lee Koloszyc</td><td><a href="m-audio/BL_MPR.rom">BL rom</a></td>
<td></td> MD5(03795b6d92c385ea0559a1ef3a03973e) </tr>

<tr><td> <a href="aldipod/JUKEBOX.mi4">Aldi Pod</a></td>FINE <td>Dave
Hooper</td> <td></td><td></td>MD5(79c23133c51b4f41f942a9b64692e3c7) </tr>

<tr><td> <a href="sirrius/S50main.mi4">Sirrius S50</a></td>FAIL <td>Lee
Koloszyc</td><td></td><td>needs BL to decode</td>
MD5(93a2d15aeeec7fba813bf3986d81e31c) </tr>

<tr><td> <a href="yh925gs/20050324100034734_FW_YH925.mi4">Samsung
YH-925GS</a></td> FINE <td></td><td><a href="yh925gs/BL_YH925.rom">BL
rom</td><td></td> MD5(18d7d69e823eacda37d5b154d2b59fb2) </tr>

<tr><td> <a href="yh820/FW_YH820.mi4">Samsung YH-820</a></td>FINE
<td></td><td></td><td></td> MD5(f40347a60d322193cb03154a7e61addb) </tr>

<tr><td> <a href="yh920/PP5020.mi4">Samsung YH-920</a></td>FINE <td></td>
<td></td><td></td>MD5(3ed1bb882128ee26dbf6420da3245a9c) </tr>

<tr><td> <a href="yh925/20050324095916500_FW_YH925.mi4">Samsung
YH-925</a></td>FINE <td>Benjamin Larsson</td>
<td></td><td></td>MD5(18d7d69e823eacda37d5b154d2b59fb2) </tr>

<tr><td> <a href="msi-p610/PP5022.mi4">MSI P610</a></td> FINE <td></td><td><a
href="msi-p610/PRI_BL_pp7005_5022_color.rom">BL rom PRI</a> + <a
href="msi-p610/SEC_BL_pp7005_5022_color.rom">BL rom SEC</a></td>
<td></td>MD5(fcb868a7773384d1ee6282b9c40de60f) </tr>

<tr><td> <a href="msi-p640/PP5022.mi4">MSI P640</a></td> FINE <td></td><td><a
href="msi-p640/BL_pp6005_5022_color.rom">BL rom</a></td>
<td></td>MD5(014848b3ed9e4e353d7b691d93311c05) </tr>

<tr><td> <a href="mrobe100/PP5020.mi4">Olympus MR-100</a> </td> FINE
 <td></td><td><a href="mrobe100/BL_pp6005_5020.rom">BL rom</a></td>
<td></td>MD5(7417ee459d86e22d795240899441ef07) </tr>

</table>

<p>
 Reminder: <tt>arm-elf-objdump -D --target binary -marm PP5022.mi4.bin</tt>
<br>
#include "ad.t"

BOXBOT

TITLE(mi4 Header Explained)
BOXTOP
<table>
<tr><th>Offset</th><th>What</th></tr>
<tr><td>0x00</td><td>32bit magic (50504f53 == "PPOS")</td></tr>

<tr><td>0x04</td><td>32bit indicating version as the 01030100 ones have bigger
 headers (0x44 bytes == includes DSA keys) than the 01020100 ones (0x1C
 bytes).
</td></tr>

<tr><td>0x08</td><td>32bit number, The length of actual data used within the
file (beyond this point the files seems to be zero-padded). However, the last
32bit word of the data always is 0xaa55aa55 and the bootloader checks it.
</td></tr>

<tr><td>0x0C</td><td>32bit crc32 (starting polynomial 0xEDB88320) over the
firwmare image starting at index 0x200 (with initial value of 0 and without
pre- and post-conditioning). This value seems to not get checked for 010301
versions. </td></tr>

<tr><td>0x10</td><td> 32bit encryption algorithm ID (or similar). The
 bootloader will skip the decryption if the value here is zero, use TEA if the
 value is two and fail if the value is any other.
</td></tr>

<tr><td>0x14</td><td>32bit file size of the mi4 (even 0x400 bytes)</td></tr>

<tr><td>0x18</td><td>32bit length of the plaintext part. The YH-920 mi4 file
 has zero there. Every other file has it 0x200. Most bootloaders seems to be
 OK with this value being the entire file's size and thus effectively removing
 encryption. </td></tr>

<tr><td>0x1C-0x2F<br>0x30-0x43</td><td> Two 160bit DSA-related numbers. They
 are the signature (referred to as r and s in the literature). The public key
 (p, q, g and y) is in the BL. The private key (x) is not disclosed anywhere.
</td></tr>
</table>

<p>
 Other details:
<ol> 
<li> there is a few "magic words" within the mi4 file that have to be correct
for the BL to accept it. TODO: list them

<li> the number at index 0x18 can be set to be the size of the entire mi4 file
and thus totally avoid having the mi4 file encrypted - this is what we do in
Rockbox when we generate mi4 files.

<li> the DSA signature can be replaced with a "dummy" (by using mi4code) which
is a fixed combination that is a known flaw in the DSA algorithm that is
accepted by the Sansa e200 BL. This allows us to run any mi4 file without
having to sign it with a "real" signature or patch the BL file to accept our
own signature. In the Rhapsody BL, they removed this flaw.
</ol>

BOXBOT

Updated: __TODAY__ __NOW__ (Central European, Stockholm Sweden)

#include "footer.t"
