#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "media.h"

#define FREQ 840
#define STACK_SIZE 24

unsigned char RAM[0x1000];

unsigned char V[16];
unsigned short I;

unsigned char DT;
unsigned char ST;

unsigned short PC=0x200;
unsigned char SP;

unsigned short STACK[STACK_SIZE];

unsigned short KEYBOARD;

unsigned char SCREEN[256];

#define CHAR_SPRITES_OFFSET 0x100
char char_sprites[80] = {
	/* Source: Cowgod's Chip-8 Technical Reference */
	/* 0 */
	0xF0,
	0x90,
	0x90,
	0x90,
	0xF0,
	/* 1 */
	0x20,
	0x60,
	0x20,
	0x20,
	0x70,
	/* 2 */
	0xF0,
	0x10,
	0xF0,
	0x80,
	0xF0,
	/* 3 */
	0xF0,
	0x10,
	0xF0,
	0x10,
	0xF0,
	/* 4 */
	0x90,
	0x90,
	0xF0,
	0x10,
	0x10,
	/* 5 */
	0xF0,
	0x80,
	0xF0,
	0x10,
	0xF0,
	/* 6 */
	0xF0,
	0x80,
	0xF0,
	0x90,
	0xF0,
	/* 7 */
	0xF0,
	0x10,
	0x20,
	0x40,
	0x40,
	/* 8 */
	0xF0,
	0x90,
	0xF0,
	0x90,
	0xF0,
	/* 9 */
	0xF0,
	0x90,
	0xF0,
	0x10,
	0xF0,
	/* A */
	0xF0,
	0x90,
	0xF0,
	0x90,
	0x90,
	/* B */
	0xE0,
	0x90,
	0xE0,
	0x90,
	0xE0,
	/* C */
	0xF0,
	0x80,
	0x80,
	0x80,
	0xF0,
	/* D */
	0xE0,
	0x90,
	0x90,
	0x90,
	0xE0,
	/* E */
	0xF0,
	0x80,
	0xF0,
	0x80,
	0xF0,
	/* F */
	0xF0,
	0x80,
	0xF0,
	0x80,
	0x80,
};

/* instructions are a 2 char array */

#define I_SHORT(instr) (short)((instr)[0]<<8u | (instr)[1])
#define I_CLASS(instr) ((instr)[0]>>4u)
#define I_NNN(instr) ((((instr)[0]&15u)<<8u) | (instr)[1])
#define I_ADDR(instr) I_NNN(instr)
#define I_NN(instr) ((instr)[1])
#define I_N(instr) ((instr)[1]&15u)
#define I_X(instr) ((instr)[0]&15u)
#define I_Y(instr) ((instr)[1]>>4u)

#define WARN(...) (fprintf(stderr, __VA_ARGS__))

static void display(){
	for(int y=0 ; y<32 ; y++){
		for(int x=0 ; x<64 ; x++){
			draw(x, y, SCREEN[y*8+x/8]&1<<(7-x%8) ? 1:0);
		}
	}
}

static void step(){
	if(PC < 0x200 || PC >= 0x1000){
		WARN("PC out of usable adress space: %X\n", (unsigned) PC);
		return; /*TODO: abort? */
	}

	unsigned char instr[2] = {RAM[PC], RAM[PC+1]};
	/* TODO: debug: print state */
	PC+=2;
	switch(I_CLASS(instr)){
		case 0:
			switch(I_SHORT(instr)){
				case 0x00E0: /* clear screen */
					for(int i=0 ; i<256 ; i++)
						SCREEN[i] = 0;
					clear_screen();
					frame();
					return;
				case 0x00EE: /* return from a subroutine */
					if(!SP)
						WARN("Stack underflow\n");
					else
						PC=STACK[--SP];
					return;
				default: /* legacy machine routine call */
					WARN("Legacy machine routine call: %X\n",
							(unsigned) I_SHORT(instr));
					return;
			}
			return;
		case 1: /* jump */
			PC=I_ADDR(instr);
			return;
		case 2: /* call a subroutine */
			if(SP==STACK_SIZE)
				WARN("Stack overflow\n");
			else
				STACK[SP++] = PC;
			PC=I_ADDR(instr);
			return;
		case 3:
			if(V[I_X(instr)] == I_NN(instr))
				PC+=2;
			return;
		case 4:
			if(V[I_X(instr)] != I_NN(instr))
				PC+=2;
			return;
		case 5:
			if(V[I_X(instr)] == V[I_Y(instr)])
				PC+=2;
			return;
		case 6:
			V[I_X(instr)] = I_NN(instr);
			return;
		case 7:
			V[I_X(instr)] += I_NN(instr);
			return;
		case 8:
			switch(I_N(instr)){
				case 0:
					V[I_X(instr)] = V[I_Y(instr)];
					return;
				case 1:
					V[I_X(instr)] |= V[I_Y(instr)];
					return;
				case 2:
					V[I_X(instr)] &= V[I_Y(instr)];
					return;
				case 3:
					V[I_X(instr)] ^= V[I_Y(instr)];
					return;
				case 4:
					V[0xf] = V[I_X(instr)] + V[I_Y(instr)] > 0xff ? 1 : 0;
					V[I_X(instr)] += V[I_Y(instr)];
					return;
				case 5:
					V[0xf] = V[I_X(instr)] > V[I_Y(instr)] ? 1 : 0;
					V[I_X(instr)] -= V[I_Y(instr)];
					return;
				case 6:
					V[0xf] = V[I_X(instr)]&1u;
					V[I_X(instr)] >>= 1u;
					return;
				case 7:
					V[0xf] = V[I_X(instr)] < V[I_Y(instr)] ? 1 : 0;
					V[I_X(instr)] = V[I_Y(instr)] - V[I_X(instr)] ;
					return;
				case 0xE:
					V[0xf] = V[I_X(instr)]>>7u;
					V[I_X(instr)] <<= 1u;
					return;

				default: /* unknown instruction */
					WARN("Unknown instruction: %X\n",
							(unsigned) I_SHORT(instr));
					return;
			}
			return;
		case 9:
			if(V[I_X(instr)] != V[I_Y(instr)])
				PC+=2;
			return;
		case 0xA:
			I=I_ADDR(instr);
			return;
		case 0xB:
			PC = I_ADDR(instr) + V[0];
			return;
		case 0xC:
			V[I_X(instr)] = rand() & I_NN(instr);
			return;
		case 0xD: /* display */
			if(I+I_N(instr) >= 0x1000){
				WARN("Sprite data out of memory bounds: %X..%X\n",
						(unsigned)I, (unsigned)(I+I_N(instr)));
				return;
			}

			V[0xf] = 0;
			unsigned char x = V[I_X(instr)];
			unsigned char y = V[I_Y(instr)];
			unsigned char shift = x%8;

			for(unsigned i=0;i<I_N(instr);i++){
				unsigned char wrapped_y = (y+i)%32;
				unsigned char *screen_tile = &SCREEN[wrapped_y*8+(x/8)%8];
				unsigned char sprite_tile = RAM[I+i]>>shift;
				V[0xf] |= *screen_tile & sprite_tile ? 1:0;
				*screen_tile ^= sprite_tile;
				if(shift){
					screen_tile = &SCREEN[wrapped_y*8+(x/8+1)%8];
					sprite_tile = RAM[I+i]<<(8u-shift);
					V[0xf] |= *screen_tile & sprite_tile ? 1:0;
					*screen_tile ^= sprite_tile;
				}
			}

			/*TODO: maybe update only written regions */
			display();
			return;

		case 0xE:
			switch(I_NN(instr)){
				case 0x9E:
					if(V[I_X(instr)] > 16)
						WARN("Unknown key: %u\n",
								(unsigned)V[I_X(instr)]);
					if(KEYBOARD & 1<<(V[I_X(instr)]))
						PC+=2;
					return;
				case 0xA1:
					if(V[I_X(instr)] > 16)
						WARN("Unknown key: %u\n",
								(unsigned)V[I_X(instr)]);
					if(!(KEYBOARD & 1<<(V[I_X(instr)])))
						PC+=2;
					return;
				default: /* unknown instruction */
					WARN("Unknown instruction: %X\n",
							(unsigned) I_SHORT(instr));
					return;
				
			}
			return;
		case 0xF:
			switch(I_NN(instr)){
				case 0x07:
					V[I_X(instr)]=DT;
					return;
				case 0x0A:
					if(!KEYBOARD)
						PC-=2;
					else {
						V[I_X(instr)]=0;
						for(unsigned short k=KEYBOARD;!(k&1);k>>=1)
							V[I_X(instr)]++;
					}
					return;
				case 0x15:
					DT=V[I_X(instr)];
					return;
				case 0x18:
					ST=V[I_X(instr)];
					return;
				case 0x1E:
					I+=V[I_X(instr)];
					/* Undocumented feature, reported on Wikipedia */
					V[0xf] = I >= 0x1000 ? 1 : 0;
					I &= 0xfff;
					return;
				case 0x29:
					if(V[I_X(instr)]>0xF)
						WARN("Too large input for a digit: %X\n",
								(unsigned)V[I_X(instr)]);
					else
						I = CHAR_SPRITES_OFFSET + 5*V[I_X(instr)];
					return;
				case 0x33:
					if(I<0x200 || I+3 >= 0x1000){
						WARN("BCD store out of memory bounds: %X..%X\n",
								(unsigned)I, (unsigned)(I+3));
						return;
					}

					RAM[I+2] = V[I_X(instr)]%10;
					RAM[I+1] = (V[I_X(instr)]/10)%10;
					RAM[I]   = (V[I_X(instr)]/100)%10;
					return;
				case 0x55:
					if(I<0x200 || I+I_X(instr) >= 0x1000){
						WARN("Register store out of memory bounds: %X..%X\n",
								(unsigned)I,
								(unsigned)(I+I_X(instr)));
						return;
					}

					for(unsigned i=0 ; i<=I_X(instr) ; i++)
						RAM[I+i] = V[i];
					return;
				case 0x65:
					if(I+I_X(instr) >= 0x1000){
						WARN("Register load out of memory bounds: %X..%X\n",
								(unsigned)I,
								(unsigned)(I+I_X(instr)));
						return;
					}

					for(unsigned i=0 ; i<=I_X(instr) ; i++)
						V[i] = RAM[I+i];
					return;
				default: /* unknown instruction */
					WARN("Unknown instruction: %X\n",
							(unsigned) I_SHORT(instr));
					return;
			}
			return;
		default: /* unreachable */
			/*TODO: assert */
			return;
	}

}

int main(int argc, char **argv){
	/* Set up char sprites */	
	memcpy(RAM+CHAR_SPRITES_OFFSET, char_sprites, sizeof(char_sprites));

	/* Load ROM */
	if(argc<2)
		return 1;
	FILE *rom=fopen(argv[1], "r");
	if(!rom)
		return 2;
	fread(RAM+0x200,1,0x1000-0x200,rom);
	if(ferror(rom))
		return 3;
	fclose(rom);

	/* Init media stuff: graphics, input, sound */
	m_init(argc, argv);

	/* Let's go */
	for(int i=0 ; 1 ; i++){
		KEYBOARD = get_input(KEYBOARD);
		if(KEYBOARD == (unsigned short)-1)
			break;

		step();
		
		if(!(i%(FREQ/60))){
			frame();
			if(DT)
				DT--;
			if(ST)
				ST--;
		}

		set_buzzer_state(ST ? 1 : 0);

		printf("i=%5d, DT=%3d, K=[", i, (int)DT);
		for(int k=0 ; k<16;k++)
			printf("%c", KEYBOARD & 1<<k ? "0123456789ABCDEF"[k]:'.');
		printf("]      \r");
	}

	/* Quit media */
	m_quit();

	return 0;
}
