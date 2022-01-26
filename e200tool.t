#include "doctype.t"
#include "setup.t"
#include "daniel.t"
HEAD(e200tool)
#include "body.t"
#include "topmenu.t"

TITLE(General)
BOXTOP
<p>
 e200tool is written by <a href="mrh.html">MrH</a>
<p>
 The e200tool is a lowlevel tool to fiddle with Sansa e200 and c200 when
started in the USB modes we call <i>pre-boot</i> and <i>manufacturing</i>.

<p>
 <a href="e200tool/e200tool-src.zip">v0.2.3 Source code</a> (<a
href="e200tool/Makefile">Makefile</a>)

<p>
<a href="e200tool/e200tool">Linux binary</a>

<p> Windows executable link removed since it hardly works for anyone.  People
have a hard time to get this to work on Windows. You need libusb installed,
but that's where my skills ends. On Linux this seems to work a lot easier.

<p>
<b>
DO NOT USE THIS TOOL UNLESS YOU KNOW WHAT YOU ARE DOING! WE TAKE NO
RESPONSIBILITY NO MATTER WHAT BAD THINGS MIGHT HAPPEN TO YOUR
PLAYER/COMPUTER/WHATEVER! IF THIS MAKES YOU UNEASY, PLEASE DELETE THIS TOOL
NOW! </b>

BOXBOT

TITLE(Restoring I2C boot rom on Sansa e200)
BOXTOP

<p><b>a few i2c rom dumps</b><br>
<a href="e200tool/i2c.bin">i2c.bin</a> Euro e260<br>
<a href="e200tool/i2c-e260.bin">i2c-e260.bin</a> US e260<br>
<a href="e200tool/i2c-e280.bin">i2c-e280.bin</a> US e2860

<p>
 <b>Using e200tool 0.1.3 or later:</b>

<p>
 e200tool i2cprogram i2c.bin

<div style='background: #e0e0e0; font-size: 90%;'>
<p>
 <b>Using e200tool before 0.1.3:</b>
<p>
1. Get to the recovery mode
<p>
e200tool recover BL_SD_boardSupportSD.rom (hold 'rec')
<p>
If you succeed,
<p>
2. Copy i2c.bin to the recovery 'disk'
<p>
3. Disconnect the player and hope the recovery mode writes the .bin to
  the i2c rom.
<p>

I don't know if it will, but I think it might. If it won't, renaming it to
sdbootrom.bin might help.
</div>

<p>
<b>SO IT SHOULD BE OBVIOUS THAT WHILE DOING THIS MAY (WITH SOME
LUCK) FIX THE I2C ROM, IT MIGHT VERY WELL BRICK THE PLAYER EVEN
FURTHER! DO NOT TRY THIS UNLESS NOTHING ELSE WORKS!</b>
<p>
If you succeed, the i2c ROM should now be corrected and you should now be able
to boot to either 'manufacturing mode', 'recovery mode' or to the
actual firmware.

BOXBOT

Updated: __TODAY__ __NOW__ (Central European, Stockholm Sweden)

#include "footer.t"
