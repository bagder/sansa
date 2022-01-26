#include "doctype.t"
#include "setup.t"
#include "daniel.t"
HEAD(SanDisk AMS Sansa Series)
#include "body.t"
#include "topmenu.t"

TITLE(SanDisk AMS Sansa Series)
BOXTOP
<p>

<i> Note: we initially called these models the "Sansa v2 series" but that
caused confusion since there are already several versions of the Clip and the
Fuze, and the AMS version of the m200 is called m200 v4 by SanDisk
themselves. We have thus switched to refer to Sansa players with this newer
"platform" as the AMS Sansas.</i>

<p>
 Starting 2007 SanDisk released updated versions of several of their models
and they've also shipped new models based on this same revised hardware
architecture. In the <b>e200</b> and <b>c200</b> cases, they in fact call the
new ones "e200 v2" and "c200 v2" and the <b>m200</b> is "v4". They've also
shipped new models called <b>Sansa Clip</b>2 and <a
href="http://www.sandisk.com/sansafuze/">Sansa Fuze</a>.

<p> These AMS models are completely changed compared to the older ones, and
they don't run the same firmwares and these new ones don't run Rockbox (yet)!

<p><b>Note</b> that the <a href="view.html">Sansa View</a> does not feature
the same chipset!

SUBTITLE(Official Discussion)
<p>
 <a href="http://forums.rockbox.org/index.php?topic=14064.0">Discussion about
this architecture and getting Rockbox to run on it!</a>

SUBTITLE(Unified Hardware)
<p>
 All these models now have the <a
href="http://www.austriamicrosystems.com/03products/products_detail/AS3525/description_AS3525.htm">AS3525</a>
from <a href="http://www.austriamicrosystems.com/">Austriamicrosystems</a>
(AMS) as their main SoC chip. They (AMS) do not offer the data sheet for this
chip publicly.

<p> Sansa Models known to be using the AMS platform:
<ul>
<li> Sansa E200 v2
<li> Sansa C200 v2
<li> Sansa M200 v4
<li> Sansa Clip
<li> Sansa Fuze
</ul>

<p> I asked AMS for the AS3525 data sheet, and I got it.

SUBTITLE(Hardware Differences e200/c200 between older and AMS version)
<p>
 These devices were previously powered by the <a
href="http://www.nvidia.com/page/pp_5024.html">PP5024</a> SoC from <a
href="http://www.nvidia.com/">Nvidia</a> (formerly PortalPlayer).

SUBTITLE(Hardware Differences m200 between older and AMS version)
<p>
 This devices was previously powered by a <a
href="http://www.telechips.com/product/p_024.asp">TCC770</a> SoC from <a
href="http://www.telechips.com/">Telechips</a>.

SUBTITLE(Firmware Images)

<p>
 We have a growing collection of various firmware images to help reverse
engineering and research. This <a href="amsfw.html">firmware collection</a> is
now on <a href="amsfw.html">its own separate page</a>.

SUBTITLE(Firmware File Format)
<p>
  This description has not been updated to reflect the latest info but is kept
  around still to be updated in the future.
<pre>
index   description (value is 32bit unless otherwise noted)

0x0     0 or 1 (seems to be the index of the header structure. 0 for 0x0000
        and 1 for the copy at 0x200)
0x4     checksum

        32bit unsigned simple additions of all values from 0x400. The data
        range for this operation is specified in the 0x0c value.
        See <a href="v2/checksum.txt">checksum.c</a>

0x8     value
        seems to be limited to only a small range: (e4/e8/ed/ea/ec) although
        not target type since images for the same target can have different
        values.

        seems to be used in the calculation of the regular blocks size: [block
        multiplier] * 0x200 = [regular block size]

0xc     Size of the first block and thus also the checksum data range, the
        amount of data to use to calculate the 0x04 checksum. Counted from
        offset 0x400

0x10    0x03

0x14    8 bit checksum2 ?

0x15    Model identifier?
        Theory that currently works:
        0x1e Fuze
        0x22 clip
        0x23 c200
        0x24 e200
        0x25 m200

0x16    16 bit zero
0x18    0x40
0x1c    0x01

0x3c    0x5000 (in the e200 alone)

0x200   a mirror copy of 0x0000-0x1ff
        as described with the index 0 value changed

0x400   Start of the first "block", as we can see ARM exception vectors and
        code

[...]

ends with "de ad be ef" padding
&nbsp;
some images (?) ends with a trailing 4 byte checksum (a simple sum of each
32-bit word in the file, not including those last 4 bytes)
</pre>

<p>
 There seems to be multiple "blocks" in the binary files, each one an even
0x200 bytes size. It seems that perhaps there's only a checksum for the first
block.
<p>
 For each regular block (that is all blocks that follows the first one), a
block header is also present right at offset 0x0 in each block. The block
header structure can be incomplete, but the following is always present:

<pre>

Offset  Value(s)        Description

0x00    String offset   Block offset to the string describing the block's
                        content

0x04    Lower address   Seems to be the lower bound of an address range, still
                        unknown

0x08    Upper address   Seems to be the upper bound of an address range, still
                        unknown

0x0C    Block size      Actual size of the block "effective" data, that is
                        where there is valid code or data

</pre>
<a name="recovery"></a>
SUBTITLE(Recovery Mode)
<p>
 We have not yet been able to dicover any <i>Recovery Mode</i> why we don't
know how we should be able to run buggy test code and recover back from that!
<p>
 The AS3525 chip itself has no documented recovery mode, so it would have to
 be provided by the bootloader itself.

SUBTITLE(Special Mode)
<p>
<b>c200v2 special mode</b> <small>(Davide Del Vento figured this out)</small>
<p>
<ol>
<li> be sure that the device is off and disconnected from the PC
<li> set the hold/lock switch on LOCK (orange visible)
<li> hold the "reverse" button (i.e. left or "<<"). Note that the device
     must still be off and not connected)
<li> connect the usb cable, WHILE STILL holding the "reverse" button
</ol>

Now it seems you've booted the device into a "special mode" that makes the
player show some directories empty and there have appeared a few "extra
files"!

These extra files are binary ones, and <a
href="/sansa/v2/c200/sansa-c200v2.zip">available here</a> (taken from a c200v2
2GB version, a mere 3.2KB zip).

<a name="test"></a>
SUBTITLE(Test Mode)
<p>
 There seems to exist some kind of test mode on the AMS models that is
activated if you put a firmware file on the target using special names. The
details have been posted in the Rockbox forums.

SUBTITLE(Firmware Upgrade)
<p>
 To upgrade to a custom firmware image, at least on the e200v2 players you
simply put the firmware image (named e200pa.bin) in the root directory and
then unplug USB.

BOXBOT

Updated: __TODAY__ __NOW__ (Central European, Stockholm Sweden)

#include "footer.t"
