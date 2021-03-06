#include "doctype.t"
#include "setup.t"
#include "daniel.t"
HEAD(The mi4code tool)
#include "body.t"
#include "topmenu.t"

TITLE(mi4code)
BOXTOP

<p> <a href="mi4code.c">mi4code.c</a> (<a href="mi4codec.html">view it</a>) is
the (de/en)coder source code provided by <a href="mrh.html">MrH</a> (Version
1.1.2, uploaded April 2008)

<p><b> All use of this code is entirely on your own risk. Everything just might
explode and there's no one but yourself to blame.</b>
BOXBOT

TITLE(Rockbox and mi4)
BOXTOP
<p>
 The Rockbox build system now uses the "standard" scramble tool with the <a
href="http://svn.rockbox.org/viewvc.cgi/trunk/tools/mi4.c?view=markup">mi4.c</a>
file, for building mi4 firmwares, since it doesn't need all the fancy
abilities of mi4code.

BOXBOT

TITLE(Compile the Tool)
BOXTOP
<p>

 <a href="mi4code.c">mi4code</a> is the single source file that can be
compiled to a single executable with:
<pre>
 gcc -o mi4code mi4code.c -lgcrypt
</pre>

<p> <i>(gcrypt is used for DSA and you can edit the source to disable DSA
 support and thus remove the dependency on this lib.)</i>

<h2>Windows binaries</h2>
<p>
<a href="mi4code0.9.33beta.zip">0.9.33</a> (<i>outdated!</i>)

BOXBOT

TITLE(The Commands)
BOXTOP

<p>
 mi4code has a few main commands that tells it what to do:

<ul>
 <li> <a href="#decrypt">decrypt</a>  - decrypt mi4 image
 <li> <a href="#encrypt">encrypt</a> - encrypt mi4 image
 <li> <a href="#build">build</a> - build mi4 image
 <li> <a href="#keyscan">keyscan</a> - scan file for potential keys
 <li> <a href="#sign">sign</a> - sign mi4 with DSA
 <li> <a href="#verify">verify</a> - verify DSA siganture
 <li> <a href="#blpatch">blpatch</a> - patch bootloader with custom DSA key
 <li> <a href="#hexdecode">hexdecode</a> - decode 'hex' bootloader file
 <li> <a href="#hexencode">hexencode</a> - encode 'hex' bootloader file
</ul>

<p>
 All commands take <b>-h</b> to display help, <b>-v</b> to add verbosity and
starting in 0.9.26 <b>-q</b> to make it more quiet.

<p> There's a small <a href="#howto">howto</a> at the bottom of this page
 describing how to combine the use of these commands when upgrading to your
 own firmware.

BOXBOT

<a name="decrypt"></a>
TITLE(Decrypt)
BOXTOP
<pre>
  mi4code decrypt [options] &lt;infile&gt; &lt;outfile&gt; [keyid]
&nbsp;
  mi4code decrypt [options] &lt;infile&gt; &lt;outfile&gt; [k1] [k2] [k3] [k4]
</pre>

<p> Decrypt (using TEA) an existing encrypted mi4 file. This command can be
 told to use one of the existing built-in keys, you can tell it explicitly
 what key to use or it will try all known internal keys. Use <b>decrypt -l</b>
 to list all known keys.

<p> Note that using <b>-s</b> to strip off the header can be useful if you
 want the binary blob to be understandable by a dissassembler at once or
 similar, but you want to keep the header if you intend to re-encrypt it into
 a mi4 file again.
<p> <b>options:</b>
<ul>
 <li> -l list known keys
 <li> -s strip mi4 header
</ul>

BOXBOT

<a name="encrypt"></a>
TITLE(Encrypt)
BOXTOP
<pre>
  mi4code encrypt [options] &lt;infile&gt; &lt;outfile&gt; &lt;keyid&gt;
&nbsp;
  mi4code encrypt [options] &lt;infile&gt; &lt;outfile&gt; &lt;k1&gt; &lt;k2&gt; &lt;k3&gt; &lt;k4&gt;
</pre>

<p> Encrypt (using TEA) a binary file into a proper mi4 file. This command can
 be told to use one of the existing built-in keys or you can tell it what key
 to use for encryption. Use <b>encrypt -l</b> to list all known keys.

<p> Note that this command doesn't add an mi4 header, but assumes that the
 binary file you pass as input already have one.

<p><b>options</b>
<ul>
<li> -l list known keys
<li> -n don't correct the checksum (default is 'correct if changed')
<li> -p [num/all] plain text length of the image (Added in 0.9.25)
</ul>

BOXBOT

<a name="build"></a>
TITLE(Build)
BOXTOP
<pre>
  mi4code build [options] &lt;infile&gt; &lt;outfile&gt; [offs_id]
</pre>
<p>
 This commands builds an mi4 file out of a binary lump. It means it creates the
 necessary mi4 header, pads its correctly, adds the magic in the end etc.
<p>
 The <i>offs_id</i> is a magic number used in the mi4 and it must be correct
 for the bootloader to accept the mi4. (added in 0.9.33)

<p><b>options</b>
<ul>
<li> -l list known offs_ids (added in 0.9.33)
<li> -2 make 010201 header
<li> -3 make 010301 header (default)
<li> -p [num/all] plain text length of the image (Added in 0.9.25)
</ul>

BOXBOT

<a name="keyscan"></a>
TITLE(Keyscan)
BOXTOP
<pre>
  mi4code keyscan [options] &lt;infile&gt; &lt;keyfile&gt; 
</pre>
<p>
Scans for possible TEA encryption keys for infile within the keyfile. The
infile is typically an mi4 file, and the keyfile its corresponding BL file.
BOXBOT

<a name="sign"></a>
TITLE(Sign)
BOXTOP
<pre>
  mi4code sign [options] &lt;infile&gt; &lt;outfile&gt; [keyid]
</pre>

<p> mi4 files are signed with DSA keys. Using this command you can sign your
 mi4 file with one of a few selectable keys. Use <b>-l</b> to list what keys
 that are available.

<p> NOTE: the bootloader will check that the mi4 file is "legimitately"
 signed.

<p><b>options</b>
<ul>
<li> -l list known keys
</ul>

BOXBOT

<a name="verify"></a>
TITLE(Verify)
BOXTOP
<pre>
  mi4code verify [options] &lt;infile&gt; [keyid]
</pre>

<p> Verifies the DSA signatures within the given mi4 file, or if not given
 with all known built-in keys.

<p><b>options</b>
<ul>
<li> -l list known keys
</ul>

BOXBOT

<a name="blpatch"></a>
TITLE(blpatch)
BOXTOP
<pre>
  mi4code blpatch [options] &lt;infile&gt; &lt;outfile&gt; [keyid]
</pre>

<p> Patch a bootloader (BL*.rom file) with a custom DSA key to make it accept
an mi4 file using the corresponding DSA signature.

BOXBOT

<a name="hexdecode"></a>
TITLE(Hexdecode)
BOXTOP
<pre>
  mi4code hexdecode [options] &lt;infile&gt; &lt;outfile&gt; &lt;keyid&gt; 
</pre>
<p>
 Decodes a 'hex' bootloader file. 'hex' here being the hex format that
 iriver H10 models use for their BL files.

<p><b>options</b>
<ul>
<li> -l      list available keys
<li> -s      strip hex header
</ul>

BOXBOT

<a name="hexencode"></a>
TITLE(Hexencode)
BOXTOP
<pre>
  mi4code hexencode [options] &lt;infile&gt; &lt;outfile&gt; &lt;keyid&gt; 
</pre>
<p>
 Encodes a 'hex' bootloader file. 'hex' here being the hex format that
 iriver H10 models use for their BL files.

<p><b>options</b>
<ul>
 <li> -l list available keys
 <li> -n don't correct the checksum
</ul>

BOXBOT

<a name="howto"></a>
TITLE(How To Upgrade Your mi4-based Player)
BOXTOP
<p> 1. Get an upgrade firmware image. This should be an mi4 file and
 (possibly) a BL file.

<p> 2. Figure out what key to use for decrypting the mi4 file. mi4code already
 knows a bunch, but it can also scan for keys in BL files (using
 'keyscan'). Decrypt the mi4 to a raw bin file.  (If the BL file is hex
 encoded, as some iriver H10 ones are, then you need to first 'hexdecode' the
 BL file before you can scan for encryption keys). The key used to decrypt
 this will be used again later in step 4. Example:
<pre class="quote">
 mi4code decrypt PP5022.mi4 PP5022.raw sansa
</pre>

<p> 3. Edit the bin file accordingly.

<p> 4. Generate a new mi4 file again by running mi4code encrypt using the same
 key as before. Example:
<pre class="quote">
 mi4code encrypt PP5022.raw PP5022-new.mi4 sansa
</pre>

<p> 5. If the mi4 uses a 010301 version, you must also run mi4code 'sign' to
 write a suitable DSA key in the file. SanDisk Sansa and iriver H10 are known
 to work when signed with the default key. Trying to sign a 010201 header
 version should still be ok and just not do anything. Example:
<pre class="quote">
 mi4code sign PP5022-new.mi4 PP5022-signed.mi4 dummy
</pre>

<p> 6. Now rename the output file (named PP5022-signed.mi4 if you signed it
 like the example above) to the original file name and put it on the player as
 your ordinary upgrade procedure dictates. It probably also involves copying a
 BL file to it. Then the upgrade procedure should be performed just like
 normal...

BOXBOT

Updated: __TODAY__ __NOW__ (Central European, Stockholm Sweden)

#include "footer.t"

