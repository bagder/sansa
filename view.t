#include "doctype.t"
#include "setup.t"
#include "daniel.t"
HEAD(SanDisk Sansa View)
#include "body.t"
#include "topmenu.t"

TITLE(SanDisk Sansa View)
BOXTOP
<p>
 SanDisk released a player during autumn 2007 called <a
href="http://www.sandisk.com/Products/Catalog(1364)-SanDisk_Sansa_View_MP3_Players.aspx">Sansa
View</a> which basically is a beefed up e200 with a better screen and larger
NAND.

<p>
 Contrary to <a href="v2.html">other players from SanDisk released this
autumn</a>, this player remains having the <a href="mi4.html">mi4 file
format</a> for firmware upgrades and the main SoC is PP-like although with
Nvidia GeForce magic applied.c At this point in time, it is still not quite
determined exactly what model it is. The firmware mentions "PP6110".

<p>
 They even seem to still have an AMS codec chip in there! The <a
href="http://www.austriamicrosystems.com/03products/products_detail/AS3517/description_AS3517.htm">AS3517</a>
is easily visible on pics posted <a
href="http://forums.rockbox.org/index.php?topic=13562.msg104967#msg104967">here</a>. (The
e200 and c200 both use the <a
href="http://www.austriamicrosystems.com/03products/products_detail/AS3514/description_AS3514.htm">AS3514</a>.)

<p>
 The <a href="mi4code.html">mi4code</a> tool can decrypt and encrypt the
View's firmware just fine.

SUBTITLE(Disk Layout)
<p>
<a href="http://forums.rockbox.org/index.php?topic=13562.msg109214#msg109214"> markys</a> in the forum showed us a fdisk output at it looks like:

<pre>
debian:~# fdisk -l /dev/sdb

Disk /dev/sdb: 8220 MB, 8220311552 bytes
253 heads, 62 sectors/track, 1023 cylinders
Units = cylinders of 15686 * 512 = 8031232 bytes

   Device Boot      Start         End      Blocks   Id  System
/dev/sdb1               1        1019     7984379    b  W95 FAT32
Partition 1 has different physical/logical beginnings (non-Linux?):
     phys=(0, 4, 11) logical=(0, 8, 27)
Partition 1 has different physical/logical endings:
     phys=(669, 23, 0) logical=(1018, 15, 2)
Partition 1 does not end on cylinder boundary.
/dev/sdb2            1019        1024       43008   84  OS/2 hidden C: drive
Partition 2 has different physical/logical beginnings (non-Linux?):
     phys=(157, 24, 1) logical=(1018, 15, 3)
Partition 2 has different physical/logical endings:
     phys=(679, 55, 0) logical=(1023, 137, 24)
Partition 2 does not end on cylinder boundary.
</pre>


SUBTITLE(Firmware Image)
<p>
 For your convenience, here's a small zip file collection with Sansa View firmware and bootloader images:

<ul>
 <li> <a href="sansa-view/View01.00.03.zip">01.00.03</a> (5MB).
 <li> <a href="sansa-view/view01.01.06a.zip">01.01.06a</a> (5MB) (thanks Robert Menes!)
</ul>

SUBTITLE(Hidden Partition Format)
<p>
 Similar to the e200 series, the View has its boot loader, main firmware image
 and more kept in the "hidden" second partition. At least large parts of it is
 laid out exactly as described in the Rockbox wiki page <a
 href="http://www.rockbox.org/twiki/bin/view/Main/SansaE200FirmwarePartition">SansaE200FirmwarePartition</a>

<p> <a
 href="http://forums.rockbox.org/index.php?topic=13562.msg109375#msg109375">markys</a>
 provided a second partition image dump, which I store local and offer for
 download here: <a href="sansa-view/part.gz">part.gz</a> 5.2MB gzip
 compressed.

BOXBOT

Updated: __TODAY__ __NOW__ (Central European, Stockholm Sweden)

#include "footer.t"
