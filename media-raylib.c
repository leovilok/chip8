#include <assert.h>
#include <string.h>

#include <raylib.h>

#include "media.h"

#define PIXEL_RADIUS 16

#define SCREEN_WIDTH (64*PIXEL_RADIUS)
#define SCREEN_HEIGHT (32*PIXEL_RADIUS)

#define WINDOW_NAME "CHIP-8 emulator"

#define BG_COLOR BLACK
#define FG_COLOR WHITE

#define SOUND_DEV_FREQ 48000
#define SOUND_SAMPLES 4096
#define BUZZER_FREQ 440
#define BUZZER_VOL .05

int buzzer_state;
AudioStream audio;

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

KeyboardKey keys[16] = {
	KEY_V,
	KEY_THREE, KEY_FOUR, KEY_FIVE,
	KEY_E, KEY_R, KEY_T,
	KEY_D, KEY_F, KEY_G,
	KEY_C, KEY_B,
	KEY_SIX, KEY_Y, KEY_H, KEY_N
};

static void update_audio(void){
	static bool should_buzz = 0;
	static long sample_num = 0;
	static char audio_buffer[SOUND_SAMPLES];

	if (buzzer_state)
		should_buzz = 1;

	if (IsAudioStreamProcessed(audio)){
		for(int i=0 ; i<SOUND_SAMPLES ; i++){
			if (should_buzz)
				audio_buffer[i] =
					((sample_num++*256*BUZZER_FREQ/SOUND_DEV_FREQ)
					 %256)*BUZZER_VOL;
			else
				audio_buffer[i] = 0;
		}
		UpdateAudioStream(audio, audio_buffer, SOUND_SAMPLES);
		should_buzz = 0;
	}
}

int m_init(int argc, char **argv){
	(void)argc, (void)argv;

	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_NAME);
	SetTargetFPS(60);

	InitAudioDevice();
	audio = InitAudioStream(SOUND_DEV_FREQ, 8, 1);

	PlayAudioStream(audio);

	BeginDrawing();

	return 0;
}

void m_quit(void){
	EndDrawing();

	CloseAudioStream(audio);
	CloseAudioDevice();
	
	CloseWindow();
}

unsigned short get_input(unsigned short input){
	if (WindowShouldClose())
		return -1;

	input = 0;

	for(int i=0 ; i<16 ; i++){
		if(IsKeyDown(keys[i]))
			input |= 1<<i;
	}

	return input;
}
unsigned short wait_input(unsigned short input){
	int new_input;
	
	while((new_input = get_input(input)) == input)
		send_draw();

	return new_input;
}

void clear_screen(void){
	ClearBackground(BG_COLOR);
}

void draw(int x, int y, int value){
	assert(0<=x && x<64);
	assert(0<=y && y<32);
	DrawRectangle(x*PIXEL_RADIUS, y*PIXEL_RADIUS,
			PIXEL_RADIUS, PIXEL_RADIUS,
			value ? FG_COLOR : BG_COLOR);
}

void send_draw(void){
	update_audio();

	EndDrawing();
	BeginDrawing();
}

void set_buzzer_state(int state){
	buzzer_state = state;
}

void wait_tick(void){
	//XXX: not used anymore
}
