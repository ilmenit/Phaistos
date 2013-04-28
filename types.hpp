#ifndef TYPES_H
#define TYPES_H

#include <vector>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned char byte_flags;

enum e_param_type : byte {
	E_PARAM_NONE,
	E_PARAM_CONST_VALUE,
	E_PARAM_MEM_SLOT,
	E_PARAM_CONST_SLOT,
	E_PARAM_ZP_SLOT
};

struct s_canonized_param
{
	byte value;
	e_param_type type;
};

struct s_instruction {
	byte opcode;
	union {
		short word_value;
		struct s_canonized_param canonized_param;
	};
};


enum e_state_equality {
	E_EQUAL_VALUE, // any value 0-FF
	E_EQUAL_MEM_NO, // index of memory slot
	E_EQUAL_CONST_NO, // index of const slot
	E_EQUAL_ANY, // the state depends
	E_EQUAL_NOT_USED, // not used during the emulation - not needed?
};

// this structure describes state of registers and memory after execution
struct s_state {
	e_state_equality equality;
	byte value;
};

struct sequence {
	std::vector <s_instruction> instructions;
	byte cycles;
	byte size;

	byte_flags input_flags; // flags describe registers that are used as inputs
	byte_flags output_flags; // flags describe registers that are changed in output
	std::vector <s_state> output_states;
};

struct OpcodeDef {
	byte opcode;
	char name[4];
	byte size;
	byte cycles;
	byte_flags d_inputs;
	byte_flags d_outputs;
	byte_flags d_memory;
	byte addressing;
	byte_flags usable;
};


enum e_register {
	E_REG_A,
	E_REG_X,
	E_REG_Y,
	E_REG_S,
	E_REG_P,
	E_REG_MAX,
};

#define D_NONE 0x0
#define D_A 0x1
#define D_X 0x2
#define D_Y 0x4
#define D_S 0x8
#define D_P 0x10

#define MEM_NONE 0
#define MEM_R 1
#define MEM_W 2

#define ILLEGAL     0x0
#define LEGAL       0x1
#define UNSTABLE    0x2
#define UNUSABLE    0x4

enum ADDRMODE { IMP, ACC, IMM, ADR, ABS, IND, REL, ABX, ABY, ZPG, ZPX, ZPY, INX, INY, ERR, NUM_ADDR_MODES };


#endif

