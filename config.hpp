#ifndef CONFIG_H
#define CONFIG_H

struct s_config {
	bool use_illegal_instructions;
	bool ignore_output_flags;
	byte max_memory_slots;
	byte max_const_slots;
	byte max_zero_page_slots;
	byte additional_zero_page_slots;
};

#endif

