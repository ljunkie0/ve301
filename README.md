VE301 is a tiny program acting as an internet radio. It provides
an MPD client and an SDL window to choose the radio stations among
other things.
The display mimics the scale disk of a vintage radio like the ve301.
It runs on a raspberry pi as well as on a normal desktop (though for
the latter there are better mpd clients).

It is still a work in progress and needs a lot of clean up. I also
guess there are still memory leaks and I am not sure whether the way
I use SDL is correct or optimal. On a Raspberry PI 3B (with SDL in framebuffer)
it is reasonably fast. The performance heavily depends on the config, e.g.
bump mapping slows down the most.

Building:
Go into the apropriate build folder (PC for desktop, Raspberry for Raspberry PI)
and type make.
This will probably fail as the are some dependencies to be installed. For convenience,
if you are running a debian system, you can try "make debian-dependencies-install".
This will install the dependencies
 - libsdl2-dev
 - libsdl2-image-dev
 - libsdl2-ttf-dev
 - libsdl2-gfx-dev
 - libcurl4-openssl-dev
 - dbus-1-dev
 - mpd-client-dev

For Raspberry PI you additionally need
 - the vc development files (I took them from the /opt directory of a raspbian install)
 - wiringPi library (2.60) (e.g. from http://wiringpi.com/download-and-install/)
 - You need two rotary encoders attached to the raspberry pi (one if you do not want to
   change the volume). The pins are currently hard-coded in rotaryencoder.c. You probably
   have to change them to reflect your setup.
 
Usage:
  Create a directory ~/.ve301
  Copy the sample sample.config to ~/.ve301/config
  Make sure that MPD is running on the machine indicated as mpd_host in the config (default is localhost).
  The MPD server should have a playlist called [Radio Streams].m3u. Its content is shown in the Radio submenu.

  On the desktop, use the mouse wheel to scroll through a menu, left click to activate an item / open a sub menu,
  right click to go back. Arrow keys Up and Down are used to increase/decrease volume.
  On the raspberry pi, the first button (Button A) replaces the mouse and the second (Button B) changes the volume.
  
