/*
Optimizations
Skip tests that rely on undefined flags
Skip tests that rely on undefined zero-page loads
Skip tests that store undefined register values
Stop if it is determined that the input code can be optimized
Post the source

Phase 1.
1. Get new sequence
2. Validate sequence
3. Execute sequence
4. Store results

Phase 2.
1. Get a sequence
2. Compare it with all the "longer" sequences
3. Store the result if the same
*/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <omp.h>
#include "types.hpp"
#include "seq_gen.hpp"
#include "config.hpp"

extern "C"{ 
#include "lib6502.h" 
}; 

using namespace std;

s_config global_configuration;

// This function iterates through all the programs and stores their output information
// The output information can be used to quickly compare two programs
void create_sequence_information()
{
	double start = omp_get_wtime( );

	sequence_generator seq_gen;
	seq_gen.init();

	int i;

//	#pragma omp parallel 
	{
		#pragma omp for 
		for (i=0;i<1000000;++i)
		{
			vector <byte> sequence;
			vector <s_instruction> instructions;
			seq_gen.get_next_sequence(sequence);
			if (seq_gen.convert_seq_to_instructions(sequence,instructions))
			{
				// the instructions are correct
				seq_gen.print_sequence(instructions);
			}
		}
	}

	double end = omp_get_wtime( );
	double wtick = omp_get_wtick( );

	printf_s("start = %.16g\nend = %.16g\ndiff = %.16g\n",
		start, end, end - start);

	printf_s("wtick = %.16g\n1/wtick = %.16g\n",
		wtick, 1.0 / wtick);
}


int main(void)
{
	global_configuration.use_illegal_instructions=false;
	global_configuration.ignore_output_flags=false;
	global_configuration.max_const_slots=2;
	global_configuration.max_memory_slots=2;
	global_configuration.max_zero_page_slots=2;
	global_configuration.additional_zero_page_slots=0;

	create_sequence_information();
	return 0;
}
