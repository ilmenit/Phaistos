#ifndef ENUMERATOR_H
#define ENUMERATOR_H

#include <vector>
#include <assert.h>

struct sequence_generator_opcode_info {

	byte opcode;
	// parameters
	std::vector <s_canonized_param> params_per_opcode;
public:
	void add_states(const e_param_type type, const size_t count);
};


class sequence_generator {
private:
	size_t opcode_max;

	// opcodes that sequence generator use
	std::vector <sequence_generator_opcode_info> usable_opcodes;

	// the sequence is vector of byte pairs <parameter_index, opcode_index>
	std::vector <byte> last_sequence_vector;

public:
	bool init();
	void get_next_sequence(std::vector <byte> &a_sequence);
	bool convert_seq_to_instructions(const std::vector<byte> &a_sequence, std::vector<s_instruction> &a_instructions);
	void print_sequence(const std::vector <s_instruction> &to_print);
};

#endif

