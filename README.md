
CHIP-8 Emulator
===============

This is a simple C [CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) emulator.

Build
-----

Using [SDL2](http://libsdl.org/):

~~~sh
# with GNU make
make MEDIA=sdl

# Or directly
cc chip8.c media-sdl.c -o chip8 $(sdl2-config --cflags --libs)
~~~

Using [Raylib](https://www.raylib.com/):

~~~sh
# with GNU make
make MEDIA=raylib

# Or directly
cc chip8.c media-raylib.c -o chip8 $(pkg-config --cflags --libs raylib)
~~~

Run
---

Get CHIP-8 roms, for example:

* [CHIP-8 Community Archive](https://johnearnest.github.io/chip8Archive/?sort=platform)
* [CHIP-8 Games Pack](https://www.zophar.net/pdroms/chip8/chip-8-games-pack.html)

Run the emulator:

~~~sh
./chip8 path/to/rom.c8
~~~

Details
-------

* CHIP-8 pixels are diplayed as 16px × 16px squares on screen (opens a 1024×512 window).
* Background color is pure black, foreground color is pure white.
* Buzzer uses a 440Hz saw wave.
* To quit, just close the window (or press *Escape* using the Raylib implementation).
* CHIP-8's 4×4 keypad is mapped on these keys:
	
	```
	┌───┬───┬───┬───┐
	│ 3 │ 4 │ 5 │ 6 │
	├───┼───┼───┼───┤
	│ E │ R │ T │ Y │
	├───┼───┼───┼───┤
	│ D │ F │ G │ H │
	├───┼───┼───┼───┤
	│ C │ V │ B │ N │
	└───┴───┴───┴───┘
	```
