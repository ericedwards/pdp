PDP
===

My original PDP-11 emulator.  Although it runs 2.9BSD, there are a number of inaccuracies
in the instruction emulation and the interrupt/event handing is a hack to make UNIX run.
I recommend using Bob Supnik's excellent [SIMH](https://github.com/simh/simh) for
your PDP-11 emulation needs.

## From the original README:

This is the first released version of my PDP-11 simulator.  I've been running
2.9BSD on it for a while.  It should run on most BSD-like 32-bit Unix machines
with little or no modification.

## Historical Notes:

My best guess is I worked on this from 1992-1994, eventually releasing it in 1994.
There was a PDP-11/34A at [Computer Science House](https://www.csh.rit.edu/)
that I wanted to get up again and running 2.9BSD.  Disk storage was the main problem,
CSH no longer had any Unibus controller / disk combinations that were usable.

I decided to build a controller that used a modern 3-1/2" drive.
The next step was to port the Western Digital IDE (wd) driver from one of the
other BSDs and build a new kernel.  The emulator worked well enough that I lost
interest with the real hardware.  You can see the incomplete work in the wd.c source file.

## BK-0010 Emulation and BK Unix:

Fast forward a quarter century... I've been doing some retrocomputing again lately
and got interested in UCSD Pascal and the LSI-11 based Terak.  Some searching and
I bumped into this SourceForge project:
[bk-terak-emu](http://bk-terak-emu.sourceforge.net/).
Leonid Broukhis enormously extended my original emulator to support the
Elektronika BK-0010 family and had thoughts of supporting the Terak 8510/a as well.
Excellent work and thank you for keeping my copyright intact!

Another amazing project I found is [bkunix](https://sourceforge.net/projects/bkunix/),
a Port of LSX Unix to Elektronika BK-0010 microcomputer.

Both are available on GitHub as well:
  * https://github.com/emestee/bk-emulator
  * https://github.com/sergev/bkunix
