.-----------------------------------------------------------------.
| TibiaMovie 0.2.1 Revision 2 (http://tibiamovie.sourceforge.net) |
'-----------------------------------------------------------------'

1. Description
--------------

TibiaMovie allows you to play and record movies that are very
efficient in file size. A typical .AVI or .MPG of Tibia would be
enormous compared to the file TibiaMovie records. It accomplishes
this by recording everything the Tibia Client receives from the server,
instead of capturing every pixel on your screen multiple times a second.

2. Movie Playback
-----------------

To play a movie, open Tibia and TibiaMovie. Click "Activate" in TibiaMovie.
Click "Play" in TibiaMovie. Click "Enter Game" in Tibia. Do NOT enter an
account number or password.

A list of movies will come up for you to choose from. The list will be taken
from the same directory/folder TibiaMovie is in, so put all .tmv files there.
In the regular server field of the character list will instead be the version
number that the movie was recorded in, for easy reference when choosing a
movie.

2.1. Speed
----------

You can speed up the playback by moving the Speed slider, or pause it by
moving it all the way to the left.

When in pause mode, you can click the ">" in the pause position to play back
the next frame. This allows for "frame by frame" playback, letting you pause,
getting to the right position, and taking a screenshot, for example.

3. Movie recording
------------------

To record a movie, open Tibia and TibiaMovie. Click "Activate" in TibiaMovie.
Click "Record" in TibiaMovie. By default, TibiaMovie will select an ascending
filename to record your movie to. You can, of course, change this after the
movie has finished recording, or you can choose your own filename initially
by using the "..." button to the right of "Record".

In TibiaMovie you will notice a list of login servers to choose from. If one
isn't working, or you would like to record a movie in the test server, change
the server before logging in.

Click "Enter Game" in Tibia, enter your account number and password, choose
a character to record on, and enter the game.

When you are finished recording, either press Ctrl+L in Tibia, or "Stop" in
TibiaMovie.

4. Markers
----------

When recording, you can set a "marker" at various stages of the recording
phase. When playing back, you can fast-forward (Speed 10) to the next marker
by clicking the "Go To Next Marker" button. TibiaMovie will then set the speed
to 10, and automatically set it back to 1 when the next marker is reached.
This allows you to effectively set points in your movie where you would like
your viewers to focus their attention on.

For example, if you were hunting in Mintwallin, and you left from Thais, you
could tell your audience that if they already knew the way to Mintwallin, then
to fast-forward to the next marker. Then, when you arrive at Mintwallin, you
would Add a marker. So people that already knew the way wouldn't have to wait
impatiently for you to get there, or if they sped the movie up themselves,
they wouldn't have to guess where to slow it back down.

5. Compatibility/Quirks
----------------

Since TibiaMovie was created for Tibia 7.26, it will not be compatible with
previous versions than this. It will be updated for each new release of Tibia,
however.

TibiaMovie has been tested only on Windows XP and I can't guarantee that it
will work on any previous Windows systems. If it does, great!

TibiaMovie has very limited support for TibiCam .rec movies and will play
old TibiCam .rec movies, but no guarantee or support is provided for TibiCam
movie playback in TibiaMovie.

5. Contact
----------

If you encounter a bug in TibiaMovie, please contact tibiamovie@erigol.com
with as much detail about your system, exactly what you were doing, etc. as
you can and I will try to fix it.

Please don't use this email address for support questions as you should be able
to figure out what to do from this readme! Support questions will go mostly
ignored, sorry.

Since we are hosted on SourceForge, support requests may be added at:

    http://sourceforge.net/tracker/?atid=669131&group_id=114671&func=browse

and bugs at:

    http://sourceforge.net/tracker/?group_id=114671&atid=669130

6. TODO
-------

Some things I may or may not do:

- Ability to not record the VIP list
- Ability to scramble peoples' names
- Allow markers to be named, and the name shows up when playing back

7. History of TibiaMovie
------------------------

TibiaMovie was created after much frustration from TibiCam. Whilst TibiCam is
an excellent program and intuitive, I seemed to have a lot more trouble with
TibiCam than others. Half the movies I got from WoT wouldn't work, even after
trying multiple combinations of TibiCam versions and Tibia versions.

TibiaMovie on the other hand, records which version of Tibia and TibiaMovie
a movie was recorded in in the .tmv file itself, making for easier "debugging"
if necessary. It also tells you this information when you playback the movie.

TibiaMovie was also designed for speed, and therefore was written in C using
the Windows API. There shouldn't be ANY noticable lag between playing in Tibia
and recording a movie through TibiaMovie. If there is, this is a bug!

8. Compiling the Source
-----------------------

I won't be providing much support for the compilation of TibiaMovie. If you
don't trust the .exe found on the website, then I will assume you have enough
skill to validate the source code, and compile your own .exe yourself. I can
however provide some details of my developing environment that may make it
easier for you.

TibiaMovie is compiled with BloodShed's Dev-C++ 4.9.8.0. You will probably
have to change a couple of pathnames in the TibiaMovie.dev file, namely the
line:

Linker=C:/Dev-Cpp/lib/libwsock32.a_@@_C:/Dev-Cpp/lib/libwinmm.a_@@_

is specific to my system. Make sure these paths are valid for your system.

I've provided a statically compiled zlib library, but it is possible to build
this yourself from the sources. I downloaded:

http://www.zlib.net/zlib121-dll.zip (precompiled, you can get the source from
zlib.net yourself if you don't trust zlib.net either!)

and used: libtool -D zlib1.dll -d zlib.def -l libzdll.a

Your mileage may vary!

If you have any patches you'd like me to add to TibiaMovie, please send them
to tibiamovie@erigol.com and I will consider them.

9. Legal Stuff
--------------

This program is not made, distributed, or supported by CipSoft GmbH.

Tibia(R) is a registered trademark of Stephan Börzsönyi, Guido Lübke, Ulrich
Schlott and Stephan Vogler.

Dev-C++ is Copyright (c) Bloodshed Software

BECAUSE "TibiaMovie" IS DISTRIBUTED FREE OF CHARGE, THERE IS ABSOLUTELY NO
WARRANTY PROVIDED, TO THE EXTENT PERMITTED BY APPLICABLE STATE LAW.
EXCEPT WHEN OTHERWISE STATED IN WRITING, "ERIGOL", WWW.ERIGOL.COM
AND/OR OTHER PARTIES PROVIDE "TibiaMovie" "AS IS" WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE
PROGRAM IS WITH YOU.  SHOULD THE "TibiaMovie" PROGRAM PROVE DEFECTIVE, YOU
ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

IN NO EVENT WILL "ERIGOL", WWW.ERIGOL.COM AND/OR ANY OTHER PARTY
WHO MAY MODIFY AND REDISTRIBUTE "TibiaMovie", BE LIABLE TO YOU FOR DAMAGES,
INCLUDING ANY LOST PROFITS, LOST MONIES, OR OTHER SPECIAL, INCIDENTAL
OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE
(INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
INACCURATE OR LOSSES SUSTAINED BY THIRD PARTIES OR A FAILURE OF THE
PROGRAM TO OPERATE WITH OTHER PROGRAMS) THE PROGRAM, EVEN IF YOU HAVE
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES, OR FOR ANY CLAIM BY
ANY OTHER PARTY.