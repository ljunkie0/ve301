VE301 is a tiny program acting as an internet radio. It provides
an MPD client and an SDL window to choose the radio stations among
other things.
The display mimics the scale disk of a vintage radio like the ve301.
It runs on a raspberry pi as well as on a normal desktop (though for
the latter there are better mpd clients).
The project was inspired by https://amrhein.eu/Radio2.

In addition to being an mpd client it can also display the current weather
condition and reacts to the bluetooth daemon for devices that connect as
audio source.

It is still a work in progress and needs a lot of clean up. I also
guess there are still memory leaks and I am not sure whether the way
I use SDL is correct or optimal. On a Raspberry PI 3B (with SDL in framebuffer)
it is reasonably fast. The performance heavily depends on the config, e.g.
bump mapping slows down the most.

Building:
Go into the apropriate build folder (PC for desktop, Raspberry for Raspberry PI)
and type make.
This will probably fail as there are some dependencies to be installed. For convenience,
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
 - The apropriate cross-compile tools for the architecture
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
  The name is chosen because with the old MPD client MPDroid you can add Stations via you mobile.

  On the desktop, use the mouse wheel to scroll through a menu, left click to activate an item / open a sub menu,
  right click to go back. Arrow keys Up and Down are used to increase/decrease volume.
  On the raspberry pi, the first button (Button A) replaces the mouse and the second (Button B) changes the volume.
  
  If you want the current weather conditions displayed you need to get a (free) api key from
  http://api.openweathermap.org/data/2.5/weather and provide it in the config file together with the location and the units.
  
  Some screenshots:
  
  ![ve301_simple](https://user-images.githubusercontent.com/78250997/155371809-a7ecea0a-1084-4f5c-988e-6c57ba19ff94.png)
  
  Simplest configuration.
  
  ![ve301_bump_map](https://user-images.githubusercontent.com/78250997/155372011-b641c42e-25ce-42c7-848e-12acd63da0a4.png)
  
  With bump mapping on the font
  
  ![ve301_light_400_400](https://user-images.githubusercontent.com/78250997/155372896-d4bdbd4f-3727-4b23-be30-f52bf80ea1e5.png)
  
  With light source at 400x400
  
  ![ve301_bg_image](https://user-images.githubusercontent.com/78250997/155373027-3bfb94e5-89e0-477b-a570-4f04c057aca5.png)
  
  With background image
  
  ![ve301_orange](https://user-images.githubusercontent.com/78250997/155373084-b0607b43-d39f-46f1-ae80-b60472fabe6c.png)
  
  In orange

