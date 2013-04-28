#include <stdio.h>
#include "types.hpp"
#include "seq_gen.hpp"
#include "config.hpp"
#include "opcode_names.cpp"
#include <omp.h>

using namespace std;

extern struct OpcodeDef opcode_def[256];
extern s_config global_configuration;

#define log_error(x)

void sequence_generator_opcode_info::add_states(const e_param_type type, const size_t count)
{
	s_canonized_param param;
	for (size_t i=0;i<count;++i)
	{
		param.type=type;
		assert(i<256); // to fit in param.value
		param.value=i;
		params_per_opcode.push_back(param);
	}
}

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof(*(a)))
#endif

bool sequence_generator::init()
{
	size_t op_num=_countof(opcode_def);

	// not to resize it all the time
	usable_opcodes.reserve(op_num);
	for (size_t i=0;i<op_num;++i)
	{
		if (opcode_def[i].usable & UNUSABLE)
			continue;

		bool usable=true;
		if (global_configuration.use_illegal_instructions==false && opcode_def[i].usable==ILLEGAL)
			usable=false;

		if (usable)
		{
			sequence_generator_opcode_info new_opcode_info;
			new_opcode_info.opcode=opcode_def[i].opcode;
			switch (opcode_def[i].addressing)
			{
				case IMP:
					break;
				case ACC:
					break;
				case IMM:
					new_opcode_info.add_states(E_PARAM_CONST_SLOT,global_configuration.max_const_slots);
					break;
				case ADR:
					break;
				case ABS:
				case ABX:
				case ABY:
					new_opcode_info.add_states(E_PARAM_MEM_SLOT,global_configuration.max_const_slots);
					break;
				case IND:
					new_opcode_info.add_states(E_PARAM_ZP_SLOT,global_configuration.max_zero_page_slots);
					break;
				case REL:
					break;
				case ZPG:
				case ZPX:
				case ZPY:
/*
					// + additional?
					new_opcode_info.add_states(E_PARAM_ZP_SLOT,global_configuration.max_zero_page_slots);
*/
					break;
				case INX:
				case INY:
/*
					// + additional?
					new_opcode_info.add_states(E_STATE_ZP,(e_state) (global_configuration.max_zero_page_slots+E_STATE_ZP));
*/
					break;
				case ERR:
					break;
			}
			if (new_opcode_info.params_per_opcode.empty())
				new_opcode_info.add_states(E_PARAM_NONE,1);
			usable_opcodes.push_back(new_opcode_info);
		}
	}
	// optimization for OMP critical function
	opcode_max=usable_opcodes.size()-1;
	return true;
}

bool sequence_generator::convert_seq_to_instructions(const std::vector<byte> &a_sequence, std::vector<s_instruction> &a_instructions)
{
	// convert incremental values into opcodes and return it
	size_t s=a_sequence.size();
	for (size_t i=0;i<s;i+=2)
	{
		unsigned char &param_i=last_sequence_vector[i];
		unsigned char &opcode_i=last_sequence_vector[i+1];
		sequence_generator_opcode_info &opcode_info=usable_opcodes[opcode_i];

		s_instruction new_one;
		new_one.opcode=opcode_info.opcode;

		if (opcode_info.params_per_opcode.empty())
		{
			new_one.canonized_param.type=E_PARAM_NONE;
		}
		else
		{
			new_one.canonized_param.type=opcode_info.params_per_opcode[param_i].type;
			new_one.canonized_param.value=opcode_info.params_per_opcode[param_i].value;
		}

		a_instructions.push_back(new_one);
	}
	return true;
}

// It just produces a next sequence of opcodes.
// This function is omp critical, therefore it must be as quick as possible (no checks or validation)
void sequence_generator::get_next_sequence(vector <byte> &a_sequence)
{
	if (last_sequence_vector.size()!=0 && usable_opcodes[last_sequence_vector[1]].opcode==0xFE)
		int a=1;

	#pragma omp critical
	{
		size_t i,s,i_max;	
		s=last_sequence_vector.size();
		// increase parameter and then opcode
		for (i=0;i<s;++i)
		{
			if (i%2==0)
			{
				i_max=usable_opcodes[ last_sequence_vector[i+1] ].params_per_opcode.size()-1;
			}
			else
				i_max=opcode_max;

			if (last_sequence_vector[i] < i_max)
			{
				++last_sequence_vector[i];
				break;
			}
			// zero first bytes in the sequence when the next by is increased
			memset(&last_sequence_vector[0], 0, sizeof(byte) * (i+1));
		}
		// if all the sequence of this size
		if (i==s)
		{
			if (i!=0)
				memset(&last_sequence_vector[0], 0, sizeof(byte) * s);
			// push param
			last_sequence_vector.push_back(0);
			// push opcode
			last_sequence_vector.push_back(0);
		}
		a_sequence=last_sequence_vector;
	}
}

void sequence_generator::print_sequence(const std::vector <s_instruction> &to_print)
{
	#pragma omp critical
	{
		size_t i,s;
		s=to_print.size();
		printf("T%d:",omp_get_thread_num());
		for (i=0;i<s;++i)
		{
			if (i!=0)
				printf(" | ");
			printf("(%02X) %s ",to_print[i].opcode,opcode_name[to_print[i].opcode]);
			
			switch(to_print[i].canonized_param.type)
			{
				case E_PARAM_NONE:
					printf("None");
					break;
				case E_PARAM_CONST_VALUE:
					printf("#0x");
					break;
				case E_PARAM_CONST_SLOT:
					printf("const");
					break;
				case E_PARAM_MEM_SLOT:
					printf("mem");
					break;
				case E_PARAM_ZP_SLOT:
					printf("zp");
					break;
			}
			printf("%x",to_print[i].canonized_param.value);
		}
		printf("\n");
	}
}
