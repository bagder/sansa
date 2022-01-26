#include "doctype.t"
#include "setup.t"

#include "daniel.t"
HEAD(SanDisk Sansa e200 Reverse Engineering)
#include "body.t"
#include "topmenu.t"

TITLE(SanDisk Sansa e200 Reverse Engineering)
BOXTOP
<a href="IMG_6589.JPG" title="Sansa e200 with LCD showing Rockbox bootloader text"><img align="right" src="IMG_6589_t.JPG" border="0" alt="Sansa e200 with LCD showing Rockbox bootloader text"></a>

<p>
 MrH findings:
<ul>
<li> <a href="button_interface.txt">Button interface v0.2</a>
<li> <a href="NAND_interface.txt">NAND interface</a> (updated Oct 6, 2007)
<li> <a href="Sansa_LCD_interface.txt">LCD interface</a>
<li> <a href="tuner_interface.txt">FM Tuner</a>
<li> <a href="memory_controller.txt">Memory Controller</a> (updated Oct 6, 2007)
<li> <a href="SIDMA_interface.txt">Semi-Independent DMA interface</a> (Updated Oct 13, 2007)

<li> DAC/codec is Austria Microsystems <a
href="http://www.austriamicrosystems.com/03products/products_detail/AS3514/AS3514.htm">AS3514</a>.
<li> The USB parts seems to be the equivalent of a built in USB controller
like the i.MX31 one
</ul>

<p>
The picture to the right was taken by MrH, as the first proof ever of
working LCD code on target.

<p> Daily tip: hold 'menu' for 15 seconds for (HW-based?) power down.

<pre>
$ dmesg
&nbsp;
  Vendor: SanDisk   Model: Sansa e260        Rev:     
  Type:   Direct-Access                      ANSI SCSI revision: 02
SCSI device sda: 7854080 512-byte hdwr sectors (4021 MB)
</pre>

<p> Below is my factory default partitions:
<pre>
$ fdisk -l /dev/sda
&nbsp;
Disk /dev/sda: 4021 MB, 4021288960 bytes
124 heads, 62 sectors/track, 1021 cylinders
Units = cylinders of 7688 * 512 = 3936256 bytes
&nbsp;
   Device Boot      Start         End      Blocks   Id  System
/dev/sda1               1        1017     3906270    b  W95 FAT32
Partition 1 has different physical/logical beginnings (non-Linux?):
     phys=(0, 9, 14) logical=(0, 9, 23)
Partition 1 has different physical/logical endings:
     phys=(230, 87, 49) logical=(1016, 34, 4)
Partition 1 does not end on cylinder boundary.
/dev/sda2            1017        1022       20480   84  OS/2 hidden C: drive
Partition 2 has different physical/logical beginnings (non-Linux?):
     phys=(230, 87, 50) logical=(1016, 34, 5)
Partition 2 has different physical/logical endings:
     phys=(232, 227, 59) logical=(1021, 74, 44)
Partition 2 does not end on cylinder boundary.
</pre>
<p>
 The first partition is the normal UMS disk while the second one is
magic.
<p>
 Note: the Rhapsody models don't have the second parition like this.
<br align="right">
#include "ad.t"

BOXBOT

TITLE(Second Partition)
BOXTOP
<p>
 Full and more details on Rockbox's <a
href="http://www.rockbox.org/twiki/bin/view/Main/SansaE200FirmwarePartition">Sansa
E200 Firmware Partition</a> page.

<p> The contents of the second partition is built up like:
<ul>
<li> 0 - 0x1ff header (The first 4 bytes of this dump says "PPBL", at index 4 is the size of the upcoming BL image)
<li> 0x200 -   BL image (padded with zeroes)
<li> 0x80000 - 0x801ff header (The first 4 bytes are "PPMI" and then at index 4 is the size of the upcoming mi4 image)
<li> 0x80200 - mi4 image (padded with zeroes)
<li> end of partition
</ul> 

<p> There's a NVPARAMS section. Byte 0x7810e1 determines whether a database
 update is done or not. 1 = update, 0 = don't update.

<p>
 Here's the first dd dumps of mine:<br>
 <a href="sansa-sda2.gz">sansa-sda2.gz</a> 5.3MB (US firmware)

<br> <a href="sansa-sda2-2.gz">sansa-sda2-2.gz</a> 5.3MB (I copied over the
BL* file again without an mi4 file)

<br> <a href="sansa-sda2-3.gz">sansa-sda2-3.gz</a> 5.3MB US firmware signed
with a dummy DSA sig

<p> Try <a href="cutit.c">cutit.c</a> (<a href="cutitc.html">view it</a>) to
extract the mi4 file from the partition. Use mi4code to decrypt it afterwards.

<p>
 The BL has a lot of code in ARM thumb mode. Disassemble with objdump like
this:
<pre>
  arm-elf-objdump -D --target binary -marm -Mforce-thumb BL.rom
</pre>

BOXBOT

TITLE(Running Rockbox on Sansa)
BOXTOP
<p>
<a href="http://www.rockbox.org/twiki/bin/view/Main/SansaE200Install">Install Rockbox on your Sansa e200</a>

BOXBOT

TITLE(e200tool)
BOXTOP
See <a href="e200tool.html">e200tool</a>
BOXBOT

TITLE(mi4)
BOXTOP
<p>

 On upgrade you put the <tt>PP5022.mi4</tt> on the file system root, but when
 the device upgrades to this it moves it elsewhere and removes it from the
 root filesystem. On other mi4 devices, such as the iriver H10, you put the
 mi4 file in the <tt>/system/</tt> directory and the bootloader loads it
 directly from there...
<p> It seems the Sansa moves the firmware image to a difference place, which
 is accessible through the second partition of the device. In that partition,
 at offset 0x80200, the mi4 file seems to be located.

<p> With a more recent Sansa firmware SanDisk changed the TEA encryption key
(and named their firmware file "<tt>firmware.mi4</tt>"). mi4code is updated to
decrypt both versions fine.

BOXBOT

TITLE(Sansa Rhapsody)
BOXTOP

<p> The Rhapsody version of the Sansa e200 series uses yet another key that
was successfully extracted on February 23, 2007. Use mi4code 0.9.33 or
later. They name the firmware file pp5022.mi4 (note the lower case).

<p> The Rhapsody BL does not allow a "dummy" DSA signed mi4 file to get
loaded.  This means we must either patch the existing BL to allow the loading
of the Rockbox bootloader, or we must "upgrade" the R model BL to a vanilla
Sansa model BL.

<p> Some initial tests of patching the original R BL file (with a .btl
extension) seems to indicate that installing a patch BL file on the R model is
somehow prohibited...

BOXBOT

<a name="usbmodes"></a>
TITLE(Sansa USB Modes)
BOXTOP
<p>
 The Sansas seem to have at least four (4) different USB modes in which it can
 start and allow various kinds of accesses from a host computer:
<ol>

<li> "pre-bootloader" - in which the Sansa appears as a "PortalPlayer USB
Device." - e200tool 0.0.6 and later can (hopefully) access the device in this
mode. A device might stop here due to a corrupted i2c rom.

<li> "Manufacturing" - see below, access it with e200tool. A device might
require this mode due to a corrupted bootloader.

<li> "Recovery" - normal UMS mode but with a 16MB disk in which you can
restore/upgrade mi4 and bootloader images.

<li> "Firmware" - when the normal firmware runs you get to a normal UMS mode
that exposes the whole data partition of the NAND flash.

</ol>

BOXBOT

TITLE(FM Tuner in Euro version)
BOXTOP
<p>
 It has been confirmed by SanDisk that the disabling of the FM tuner in the
 euro version of the player is done in firmware/software.

<p>
 This is contradicted by the people that claim that the euro version has no
radio chip insde... Different HW revisions doing it differently perhaps?

BOXBOT

TITLE(Recovery Mode)
BOXTOP

If you put a bad mi4 on the device it can no longer function properly, but you
must then enter Recovery Mode and correct the bad image.

<ul>
<li> Power off
<li> Turn ON lock/hold
<li> Hold down record
<li> Hit Power/Menu Button
</ul>
<p>
 Note that when I did this, the device appears like this with dmesg:
<pre>
  Vendor: SanDisk   Model: Sansa e250        Rev:     
  Type:   Direct-Access                      ANSI SCSI revision: 02
SCSI device sda: 32769 512-byte hdwr sectors (17 MB)
sda: assuming Write Enabled
sda: assuming drive cache: write through
 sda: unknown partition table
</pre>
... so there's no partitions but I had to mount /dev/sda instead of /dev/sda1.

<p>
 You then <b>only</b> copy your fine mi4 file to the player,
disconnect USB and it'll use that new mi4.

<p> It seems people have copied all sorts of things to the player in recovery
mode, and then suffered badly.

BOXBOT

TITLE(Manufacturing Mode)
BOXTOP

<p>This mode requires a special driver or tool to access it, and I just got a
complete system lock when I tried it without one!

<ul>
<li> Power off
<li> Turn ON lock/hold
<li> Hold down center select button
<li> Insert USB cable
</ul>

<i>"As of now we can not give out manufacturing mode to anyone outside sandisk
or sandisk vendors"</i> / SanDisk person

<p> <b>MrH</b> has written a tool for addressing and poking on the device
while in manufacturing mode. See the <a
href="e200tool/e200tool-src.zip">e200tool</a>.

<p><b>How to get into Manufacturing Mode on a C200</b>:
<ul>
<li> Power off
<li> Turn ON lock/hold
<li> Hold down DOWN button
<li> Insert USB cable or press menu/on while holding down button
</ul>

BOXBOT

Updated: __TODAY__ __NOW__ (Central European, Stockholm Sweden)

#include "footer.t"

