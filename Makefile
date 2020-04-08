MEDIA=sdl

LDLIBS=-l${MEDIA:sdl=SDL2}

chip8: media-${MEDIA}.o

clean:
	-$(RM) chip8 media-*.o

.PHONY: clean
