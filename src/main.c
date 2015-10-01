#include "cpu.c"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void unimplemented_instruction( state_8080* state )
{
	unsigned char *opcode = &state->memory[state->pc];
	printf("Error: Unimplemented instruction 0x%x\n",*opcode);
	exit(1);
}

uint8_t parity_two( uint8_t lhs, uint8_t rhs )
{
	return 0;
}

uint8_t parity( uint8_t value )
{
	return 0;
}

void add( state_8080* state, uint8_t value )
{
	// do the math with 16 bit precision so that we can
	// capture the carry.
      	uint16_t result = (uint16_t) state->a + (uint16_t) state->b;

	//if the result is zero set the zero flag to zero;
       	state->cc.z = result & 0xff == 0;

	//set the sign flag if the 7th bit is set
	state->cc.s = result & 0x80;

	//set the carry flag if more then 8 bits are set
	state->cc.cy = result > 0xff;

	//change back to 8 bit result as we are done reading
	//flags
	uint8_t result8 = result = 0xff;

	//set the parity flag
	state->cc.p = parity( result8 );
	       
	state->a = result8;
}

// register instruction addc
// the content of reg c is added to the content of the accumalator
void add_c( state_8080* state )
{
	add( state, state->c );
}

// register instruction addb
// the content of reg b is added to the content of the accumalator
void add_b( state_8080* state )
{
	add( state, state->b );
}

// imediate instruction adi
// add the immediate opcode to the content of the accumalator
void adi( state_8080* state, unsigned char* opcode )
{
	add( state, opcode[1] );
}

// memory instruction adm
// add a value at a memory offset to the content of the accumalator
void adm( state_8080* state )
{
	//calculate the memory offset by combining the hl registers
	uint16_t offset = ( state->h << 8 ) | state->l;
	add( state, state->memory[offset] );
}

// branch condition jmp
// jump to address provided in opcodes
void jmp( state_8080* state, unsigned char* opcode )
{
	printf("JMP $%x%x\n", opcode[2], opcode[1] );
	state->pc = ( opcode[2] << 8 ) | opcode[1];
}

int emulate_8080( state_8080* state )
{
	uint8_t* opcode = &state->memory[state->pc];
	printf( "%x ", *opcode);
	switch(*opcode)
	{
	case 0x00:
		//NOP
		printf("NOP\n");
		state->pc++;
		break;
	case 0x01:
		// LXI B, word
	{
		state->c = opcode[1];
		state->b = opcode[2];
		state->pc +=2;
	}
		break;
	case 0x0f:
	{
		// RRC, rotate right through carry
		uint8_t x = state->a;
		state->a = ((x & 1) << 7) | (x >> 1 );
		state->cc.cy = ( 1 == ( x & 1 ) );
	}
		break;
	case 0x1f:
	{
		// RAR, rotate accumalator right through carry
		uint8_t x = state->a;
		state->a = ( state->cc.cy << 7 ) | ( x >> 1 );
		state->cc.cy = ( 1 == ( x & 1 ) );
	}
		break;
	case 0x2f:
		state->a = ~state->a;
		break;
	case 0x41:
		// MOV B,C
		state->b = state->c;
		break;
	case 0x42:
		//MOV B,D
		state->b = state->d;
		break;
	case 0x43:
		//MOV B,E
		state->b = state->e;
		break;
	case 0x80:
		//ADD B
		add_b( state );
		break;
	case 0x81:
		//ADD C
		add_c( state );
		break;
	case 0x86:
		//ADD M
		adm( state );
		break;
	case 0xc1:
	{
		//POP B
		state->c = state->memory[state->sp++];
		state->b = state->memory[state->sp++];
	}
	break;
	case 0xc2:
		//JNZ branch condition jump not zero
		if( state->cc.z )
		{
			jmp(state, opcode);
		}
		else
		{
			state->pc +=2; // not jumping move on program counter
		}
		break;
	case 0xc3:
		//JMP branch condition
		jmp(state, opcode);
		break;
	case 0xc5:
	{
		// PUSH B
		state->memory[--state->sp] = state->b;
		state->memory[--state->sp] = state->c;	
	}
	break;
	case 0xc6:
		adi( state, opcode );
		break;
	case 0xc9:
		//RET
		state->pc = state->memory[ state->sp ] |
			( state->memory[ state->sp + 1 ] << 8 );
		state->sp += 2;
		break;
	case 0xcd:
	{
		//CALL
		uint16_t ret = state->pc + 2;
		state->memory[ state->sp -1 ] = (ret >> 8) & 0xff;
		state->memory[ state->sp -2 ] = ret & 0xff;
		state->sp -= 2;
		state->pc = (opcode[2] << 8) | opcode[1];
	}
	break;
	case 0xe6:
	{
		// ANI - and imediate
		uint8_t x = state->a & opcode[1];
		state->cc.z = x == 0; //zero flag
		state->cc.s = (0x80 == (x & 0x80)); //sign flag
		state->cc.p = parity_two(x, 8); // parity flag
		state->cc.cy = 0; //and imediate clears cy
		state->a = x; // set accumalator
		state->pc++;
	}
	break;
	case 0xf1:
	{
		//POP PSW
		uint8_t psw = state->memory[state->sp++];
		state->a = state->memory[state->sp++];

		state->cc.z = ( 0x01 == ( psw & 0x01 ) );
		state->cc.s = ( 0x02 == ( psw & 0x02 ) );
		state->cc.p = ( 0x04 == ( psw & 0x04 ) );
		state->cc.cy = ( 0x05 == ( psw & 0x05 ) );
		state->cc.ac = ( 0x10 == ( psw & 0x10 ) );
	}
	break;
	case 0xf5:
	{
		//PUSH PSW
		state->memory[--state->sp] = state->a;
		state->memory[--state->sp] =
			state->cc.z |
			state->cc.s << 1 |
			state->cc.p << 2 |
			state->cc.cy << 3 |
			state->cc.ac << 4;
	}
	break;
	case 0xfe:
	{
		// CPI compare imediate with accumalator
		uint8_t x = state->a - opcode[1];
		state->cc.z = x == 0;
		state->cc.s = 0x80 == ( x & 0x80 );
		state->cc.p = parity_two(x,8); //unsure
		state->cc.cy = state->a < opcode[1];
		state->pc++;
	}
	break;
	default:
		unimplemented_instruction( state );
		break;
	}
}

int main( int argc, char** argv )
{
	FILE* f = fopen( argv[1], "rb" );

	if( f == NULL )
	{
		printf( "Error: Couldn't open %s\n", argv[1]);
		exit(1);
	}

	fseek( f, 0L, SEEK_END ); //err
	int fsize = ftell( f );   //err
	fseek( f, 0L, SEEK_SET ); //err

	uint8_t* buffer = malloc( sizeof( uint8_t ) * fsize );
	fread( buffer, sizeof( unsigned char ), fsize, f ); //err

	fclose( f );

	printf("Starting\n");

	//create an empty state
	state_8080 state;

       	state.memory = buffer;
	state.pc = 0;
	//int pc = 0;
	while( state.pc < fsize )
	{
		 emulate_8080( &state );
		//printf("%c",buffer[pc++]);
	}

	free(buffer);
	return 0;
	     
}
