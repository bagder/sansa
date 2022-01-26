#include "doctype.t"
#include "setup.t"
#include "daniel.t"
HEAD(SanDisk e260 Ripped Apart)
#include "body.t"
#include "../../topmenu.t"

TITLE(SanDisk e260 Ripped Apart)
BOXTOP
<p>
<a
href="http://www.rockbox.org/twiki/bin/view/Main/SandiskE200HardwareComponents">Rockbox
SandiskE200HardwareComponents wiki page</a> gathers all details and HW
knowledge.

<p> Here's pictures of my Sansa e260 internals. I had to use my digicam since
my scanner isn't really good enough for this kind of task. I think the pics
have details enough to at least be interesting.
<p>
 There are additional photos on the <a href="../../e200-devboard.html">e260
dev board</a> page.

<p>
<a href="allpieces.jpg" title="all pieces of a ripped apart e260"><img border="0" src="t_allpieces.jpg" alt="all pieces of a ripped apart e260"></a>

<br> (Above) Here's all pieces when taken apart. The LCD is attached to the
PCB using a flat cable in the bottom end and two platic "arms" holding on to
the PCB in the upper end.

<p>
<a href="board1.jpg" title="PCB backside"><img border="0" src="t_board1.jpg" alt="PCB backside"></a>
<br>

(Above) Here's the back/under side of the PCB. The PP5024B is the middle chip,
with the Hynix (believed to be (S)DRAM) above that:

<p>
<img src="pp5024.jpg">
<img src="hynix.jpg">

<p> The flash chips (shown below) are located on the separate tiny daughter
board to the very right. As I pointed out in the wiki, the numbers of my chips
are interesting when comparing with the 6GB and 2GB models, as it seems the
'16384' chip is 2GB while the 4GB one seems to be marked '32768'... (and yes,
the pic shows my fingers and tool since I didn't want to remove the plastic
pad completely but just pushed it gently for the photo to catch the numbers).
The number would then match megabits of course. 32768/8 = 4096 and 16384/8 =
2048...  <br> <img src="flash.jpg">

<p>
<a href="board2.jpg" title="PCB upside with buttons and LCD"><img border="0" src="t_board2.jpg" alt="PCB upside with buttons and LCD"></a>
<a href="lcdpcb.jpg"><img src="t_lcdpcb.jpg" border="0"></a>

<br>

Above is the front/upper side of the PCB with the wheel thing and LCD. <br>
And a closup on the little "LC2" circuit in the lower right corner:<br> <img
src="lc2.jpg">

<p>
<a href="battery1.jpg" title="battery front side"><img border="0" src="t_battery1.jpg" alt="battery front side"></a><a href="battery2.jpg" title="battery back side"><img border="0" src="t_battery2.jpg"></a>
<br>
Above is front and back side of the stock 750mAh battery. Easily replacable.
<p>
<a href="front.jpg" title="front cover"><img alt="front cover" border="0" src="t_front.jpg"></a>
<br>
Front cover
<p>

I saved the best for last. The full 20MB <a href="e200nude.mpg">e200 nude
movie</a>. <a href="http://download.rockbox.org/movies/e200nude.mpg">video on
faster site</a>

#if 0
DSC03296.JPG
DSC03297.JPG
DSC03298.JPG
DSC03299.JPG
DSC03303.JPG
DSC03305.JPG
DSC03306.JPG
DSC03308.JPG
DSC03309.JPG
DSC03310.JPG
DSC03311.JPG
DSC03312.JPG
DSC03313.JPG
DSC03314.JPG
#endif

BOXBOT

#include "footer.t"

