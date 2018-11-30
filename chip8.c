#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

unsigned char RAM[0x1000];

unsigned char V[16];
unsigned short I;

unsigned char DT;
unsigned char ST;

unsigned short PC=0x200;
unsigned char SP;

unsigned short STACK[16];

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
#define I_KK(instr) I_NN(instr)
#define I_N(instr) ((instr)[1]&15u)
#define I_NIBBLE(instr) I_N(inst)
#define I_X(instr) ((instr)[0]&15u)
#define I_Y(instr) ((instr)[1]>>4u)

/*XXX:debug*/
void display();

void step(){
	unsigned char instr[2] = {RAM[PC], RAM[PC+1]};
	/* TODO: debug: print state */
	PC+=2;
	switch(I_CLASS(instr)){
		case 0:
			switch(I_SHORT(instr)){
				case 0x00E0: /* clear screen */
					for(int i=0 ; i<256 ; i++)
						SCREEN[i] = 0;
					break;
				case 0x00EE: /* return from a subroutine */
					if(SP)
						PC=STACK[--SP];
					break;
				default: /*legacy instr*/
					/*TODO: warn*/
					break;
			}
			break;
		case 1: /* jump */
			PC=I_ADDR(instr);
			break;
		case 2: /* call a subroutine */
			STACK[SP++] = PC;
			PC=I_ADDR(instr);
			break;
		case 3:
			if(V[I_X(instr)] == I_NN(instr))
				PC+=2;
			break;
		case 4:
			if(V[I_X(instr)] != I_NN(instr))
				PC+=2;
			break;
		case 5:
			if(V[I_X(instr)] == V[I_Y(instr)])
				PC+=2;
			break;
		case 6:
			V[I_X(instr)] = I_NN(instr);
			break;
		case 7:
			V[I_X(instr)] += I_NN(instr);
			break;
		case 8:
			switch(I_N(instr)){
				case 0:
					V[I_X(instr)] = V[I_Y(instr)];
					break;
				case 1:
					V[I_X(instr)] |= V[I_Y(instr)];
					break;
				case 2:
					V[I_X(instr)] &= V[I_Y(instr)];
					break;
				case 3:
					V[I_X(instr)] ^= V[I_Y(instr)];
					break;
				case 4:
					V[0xf] = V[I_X(instr)] + V[I_Y(instr)] > 0xff ? 1 : 0;
					V[I_X(instr)] += V[I_Y(instr)];
					break;
				case 5:
					V[0xf] = V[I_X(instr)] > V[I_Y(instr)] ? 1 : 0;
					V[I_X(instr)] -= V[I_Y(instr)];
					break;
				case 6:
					V[0xf] = V[I_X(instr)]&1u;
					V[I_X(instr)] >>= 1u;
					break;
				case 7:
					V[0xf] = V[I_X(instr)] < V[I_Y(instr)] ? 1 : 0;
					V[I_X(instr)] = V[I_Y(instr)] - V[I_X(instr)] ;
					break;
				case 0xE:
					V[0xf] = V[I_X(instr)]>>7u;
					V[I_X(instr)] <<= 1u;
					break;



				default: /*unknown instruction*/
					/*TODO: warn*/
					break;
			}
			break;
		case 9:
			if(V[I_X(instr)] != V[I_Y(instr)])
				PC+=2;
			break;
		case 0xA:
			I=I_ADDR(instr);
			break;
		case 0xB:
			PC = I_ADDR(instr) + V[0];
			break;
		case 0xC:
			V[I_X(instr)] = rand() & I_NN(instr);
			break;
		case 0xD: /*display*/
			/*TODO:check OOB coords*/
			/*assert(Vx in [0;63], Vy in [0;31]), Vy+nible in [0;31]*/
			V[0xf] = 0;
			unsigned char x = V[I_X(instr)];
			unsigned char y = V[I_Y(instr)];
			unsigned char shift = x%8;

			for(int i=0;i<I_N(instr);i++){
				unsigned char *screen_tile = &SCREEN[(y+i)*8+x/8];
				unsigned char sprite_tile = RAM[I+i]>>shift;
				V[0xf] |= *screen_tile & sprite_tile ? 1:0;
				*screen_tile ^= sprite_tile;
				if(shift){
					screen_tile = &SCREEN[(y+i)*8+(x/8+1)%8];
					sprite_tile = RAM[I+i]<<(8u-shift);
					V[0xf] |= *screen_tile & sprite_tile ? 1:0;
					*screen_tile ^= sprite_tile;
				}
			}
			/*XXX:debug*/
			display();
			//usleep(500000);
			break;
		case 0xE:
			switch(I_NN(instr)){
				case 0x9E:
					if(KEYBOARD & 1<<(V[I_X(instr)]))
						PC+=2;
					break;
				case 0xA1:
					if(!(KEYBOARD & 1<<(V[I_X(instr)])))
						PC+=2;
					break;
				default: /*unknown instruction*/
					/*TODO: warn*/
					break;
				
			}
			break;
		case 0xF:
			switch(I_NN(instr)){
				case 0x07:
					V[I_X(instr)]=DT;
					break;
				case 0x0A:
					if(!KEYBOARD)
						PC-=2;
					else {
						V[I_X(instr)]=0;
						for(unsigned char k=KEYBOARD;!(k&1);k>>=1)
							V[I_X(instr)]++;
					}
					break;
				case 0x15:
					DT=V[I_X(instr)];
					break;
				case 0x18:
					ST=V[I_X(instr)];
					break;
				case 0x1E:
					I+=V[I_X(instr)];
					break;
				case 0x29:
					/*TODO: warn OOB*/
					if(V[I_X(instr)]<=0xF)
						I = CHAR_SPRITES_OFFSET + 5*V[I_X(instr)];
					break;
				case 0x33:
					RAM[I+2] = V[I_X(instr)]%10;
					RAM[I+1] = (V[I_X(instr)]/10)%10;
					RAM[I]   = (V[I_X(instr)]/100)%10;
					break;
				case 0x55:
					for(int i=0 ; i<I_X(instr) ; i++)
						RAM[I+i] = V[i];
					break;
				case 0x65:
					for(int i=0 ; i<I_X(instr) ; i++)
						V[i] = RAM[I+i];
					break;
				default: /*unknown instruction*/
					/*TODO: warn*/
					break;
			}
			break;
		default: /*unknown instr*/
			/*TODO: warn*/
			break;
	}

}

void display(){
	printf("\033[H");
	for(int y=0 ; y<32 ; y++){
		for(int xc=0 ; xc<8 ; xc++){
			for(int xb=7 ; xb>=0 ; xb--){
				printf("%s",SCREEN[y*8+xc]&1<<xb ? "█":" ");
			}
		}
		printf("\n");
	}
}

int main(int argc, char **argv){

	/* Set up char sprites */	
	for(int i=0 ; i<sizeof(char_sprites) ; i++)
		RAM[CHAR_SPRITES_OFFSET+i] = char_sprites[i];

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

	/*TODO: init more stuff: graphics, input, sound, timer*/

	/* Let's go */
	for(int i=0 ; 1 ; i++){
		step();
		/*XXX:debug*/
		/*TODO: other stuff*/
		//display();
		nanosleep(&(struct timespec){0,1000000},NULL);
		if(!(i%16)){
			//display();
			if(DT)
				DT--;
			if(ST)
				ST--;
		}
		printf("i=%5d, DT=%3d         \r", i, (int)DT);
	}

	return 0;
}
