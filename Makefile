MEDIA=sdl

LIBS_MINIAUDIO=-ldl -lm -lpthread

LIBS_sdl=-lSDL2
LIBS_raylib=-lraylib
LIBS_glfw=-lglfw -lGLESv2 ${LIBS_MINIAUDIO}


LDLIBS=${LIBS_${MEDIA}}

chip8: media-${MEDIA}.o

clean:
	-$(RM) chip8 media-*.o

.PHONY: clean
