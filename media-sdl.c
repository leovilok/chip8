#include <assert.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "media.h"

#define PIXEL_RADIUS 16

#define SCREEN_WIDTH (64*PIXEL_RADIUS)
#define SCREEN_HEIGHT (32*PIXEL_RADIUS)

#define WINDOW_NAME "CHIP-8 emulator"

#define NUM_PIXELS (SCREEN_WIDTH*SCREEN_HEIGHT)

#define SOUND_DEV_FREQ 48000
#define SOUND_SAMPLES 1024
#define BUZZER_FREQ 440
#define BUZZER_VOL .05

unsigned char *pixels;

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
SDL_Rect rect = {.x=0,.y=0,.w=SCREEN_WIDTH,.h=SCREEN_HEIGHT};

/* sound state*/
int buzzer_state = 0;
long sample_num = 0;

unsigned clock;

/*
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ C │
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ D │
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ E │
├───┼───┼───┼───┤
│ A │ 0 │ B │ F │
└───┴───┴───┴───┘
*/

SDL_Keycode keys[16] = {
	SDLK_v,
	SDLK_3, SDLK_4, SDLK_5,
	SDLK_e, SDLK_r, SDLK_t,
	SDLK_d, SDLK_f, SDLK_g,
	SDLK_c, SDLK_b,
	SDLK_6, SDLK_y, SDLK_h, SDLK_n
};

static void buzzer_callback(void* userdata, Uint8* stream, int len){
	(void) userdata;
	for(int i=0 ; i<len ; i++){
		if(buzzer_state == 0)
			stream[i] = 0;
		else /* saw waves at the moment */
			stream[i] =
				((sample_num++*256*BUZZER_FREQ/SOUND_DEV_FREQ)
				 %256)*BUZZER_VOL;
	}
}

int m_init(int argc, char **argv){
	(void)argc, (void)argv;
	/*TODO: check every init */
	
	pixels = calloc(1,NUM_PIXELS);
	
	SDL_Init( SDL_INIT_EVERYTHING );
	window = SDL_CreateWindow(
			WINDOW_NAME,
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			SCREEN_WIDTH, SCREEN_HEIGHT,
			0);

	renderer = SDL_CreateRenderer(
			window, -1,
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	texture = SDL_CreateTexture(
			renderer,
			SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_STREAMING,
			SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_RenderClear(renderer);

	SDL_OpenAudio(&(SDL_AudioSpec){
			.freq = SOUND_DEV_FREQ,
			.format = AUDIO_U8,
			.channels = 1,
			.samples = SOUND_SAMPLES,
			.callback = buzzer_callback,
			}, NULL);

	SDL_PauseAudio(0);

	clock = SDL_GetTicks();	

	return 0;
}

void m_quit(void){
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_CloseAudio();

	SDL_Quit();

	free(pixels);
}

unsigned short get_input(unsigned short input){
	SDL_Event e;

	while(SDL_PollEvent(&e)){
		if (e.type == SDL_QUIT) {
			return -1;
		}
		if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
			for(int i=0;i<16;i++){
				if(e.key.keysym.sym == keys[i]){
					if(e.type == SDL_KEYDOWN){
						input |= 1<<i;
					} else {
						input &= ~(1<<i);
					}
					break;
				}
			}
		}
	}

	return input;
}
unsigned short wait_input(unsigned short input){
	/* TODO: better version */
	int new_input;
	
	while((new_input = get_input(input)) == input)
		SDL_Delay(1);

	return new_input;
}

void clear_screen(void){
	memset(pixels,0,NUM_PIXELS);
}

void draw(int x, int y, int value){
	assert(0<=x && x<64);
	assert(0<=y && y<32);

	for(int i=0 ; i<PIXEL_RADIUS ; i++){
		for(int j=0 ; j<PIXEL_RADIUS ; j++){
			pixels[(y*PIXEL_RADIUS+i)*SCREEN_WIDTH +
				x*PIXEL_RADIUS+j] = value ? 255 : 0;
		}
	}
}

void send_draw(void){
	void *texture_pixels;
	int pitch; /* we don't actually use this, yolo */
	SDL_LockTexture(texture, &rect, &texture_pixels, &pitch);
	memcpy(texture_pixels, pixels, NUM_PIXELS);
	SDL_UnlockTexture(texture);

	SDL_RenderCopy(renderer, texture, &rect, &rect);
	SDL_RenderPresent(renderer);
	SDL_RenderClear(renderer);
}

void set_buzzer_state(int state){
	buzzer_state = state;
}

void wait_tick(void){
	/* TODO: tunable frequency */
	unsigned new_clock = SDL_GetTicks();

	if(new_clock == clock){
		SDL_Delay(1);
	}

	clock = SDL_GetTicks();
}
