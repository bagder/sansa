index   description (value is 32bit unless otherwise noted)

0x0     0
0x4     checksum?
0x8     type? (e4/e8/ed/ea)
0xc     some kind of value that is hardly a checksum as the value differ in
        the four files only between 0x01c6ac and 0x01d854. Possibly it is a
        16 bit value with 0x0e being another 16bit value always set to 0x01!
0x10    0x03
0x14    16 bit checksum3 ?

        Interesting detail about the byte at 0x15: it is 0x22, 0x23, 0x24 and
        0x25 in the files we have...

0x16    16 bit zero
0x18    0x40
0x1c    0x01

0x3c    0x5000 (in the e200 alone)

0x200   0x01
0x204-0x021f same as 0x04-0x1f

0x023c  same as 0x3c (in the e200 alone)

------------------------------------------------------------------------------
C200

00000000  00 00 00 00 2f fe ae 55  e8 00 00 00 3c ce 01 00  |..../..U....<...|
00000010  03 00 00 00 36 23 00 00  40 00 00 00 01 00 00 00  |....6#..@.......|
00000020  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
*
00000200  01 00 00 00 2f fe ae 55  e8 00 00 00 3c ce 01 00  |..../..U....<...|
00000210  03 00 00 00 36 23 00 00  40 00 00 00 01 00 00 00  |....6#..@.......|
00000220  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
*
00000400  58 f0 9f e5 58 f0 9f e5  58 f0 9f e5 58 f0 9f e5  |X...X...X...X...|

E200

00000000  00 00 00 00 50 fe 97 e6  ed 00 00 00 54 d8 01 00  |....P.......T...|
00000010  03 00 00 00 4d 24 00 00  40 00 00 00 01 00 00 00  |....M$..@.......|
00000020  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
00000030  ff ff ff ff ff ff ff ff  ff ff ff ff 00 50 00 00  |.............P..|
00000040  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
*
00000200  01 00 00 00 50 fe 97 e6  ed 00 00 00 54 d8 01 00  |....P.......T...|
00000210  03 00 00 00 4d 24 00 00  40 00 00 00 01 00 00 00  |....M$..@.......|
00000220  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
00000230  ff ff ff ff ff ff ff ff  ff ff ff ff 00 50 00 00  |.............P..|
00000240  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|

M200

00000000  00 00 00 00 bc c6 d7 0b  e4 00 00 00 ac c6 01 00  |................|
00000010  03 00 00 00 c4 25 00 00  40 00 00 00 01 00 00 00  |.....%..@.......|
00000020  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
*
00000200  01 00 00 00 bc c6 d7 0b  e4 00 00 00 ac c6 01 00  |................|
00000210  03 00 00 00 c4 25 00 00  40 00 00 00 01 00 00 00  |.....%..@.......|
00000220  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|

Clip

00000000  00 00 00 00 7c ff 2a bc  ea 00 00 00 28 d2 01 00  |....|.*.....(...|
00000010  03 00 00 00 a1 22 00 00  40 00 00 00 01 00 00 00  |....."..@.......|
00000020  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
*
00000200  01 00 00 00 7c ff 2a bc  ea 00 00 00 28 d2 01 00  |....|.*.....(...|
00000210  03 00 00 00 a1 22 00 00  40 00 00 00 01 00 00 00  |....."..@.......|
00000220  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
