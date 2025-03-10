/**
 * @file sequence_generator.cpp
 * @brief Implementation of the sequence generator
 */
#include "sequence_generator.hpp"
#include "logger.hpp"

namespace phaistos {

SequenceGenerator::SequenceGenerator()
    : current_length(1), max_length(32), current_combination_index(0) {
    
    initOpcodeInfo();
    valid_opcodes = getAllValidOpcodes();
    reset();
}

void SequenceGenerator::setMaxLength(size_t max_length) {
    this->max_length = max_length;
    if (current_length > max_length) {
        reset();
    }
}

void SequenceGenerator::setValidOpcodes(const std::vector<uint8_t>& opcodes) {
    this->valid_opcodes = opcodes;
    reset();
}

void SequenceGenerator::reset() {
    current_length = 1;
    current_combination_index = 0;
    current_sequences.clear();
    generateSequencesOfLength(current_length);
}

bool SequenceGenerator::next(std::vector<uint8_t>& sequence) {
    if (current_combination_index >= current_sequences.size()) {
        // Move to next length
        current_length++;
        current_combination_index = 0;
        
        if (current_length > max_length) {
            return false; // Done generating
        }
        
        // Generate sequences of the new length
        generateSequencesOfLength(current_length);
        
        // Check if we have any sequences
        if (current_sequences.empty()) {
            return false;
        }
    }
    
    // Return next sequence
    sequence = current_sequences[current_combination_index++];
    return true;
}

void SequenceGenerator::generateSequencesOfLength(size_t length) {
    Logger& logger = getLogger();
    logger.debug("Generating sequences of length " + std::to_string(length));
    
    current_sequences.clear();
    
    // Start with empty sequence
    std::vector<uint8_t> base;
    generateSequencesRecursive(base, length);
    
    logger.debug("Generated " + std::to_string(current_sequences.size()) + " raw sequences");
    
    // Apply pruning
    logger.debug("Applying pruning rules");
    pruneInvalidSequences();
    logger.debug("After pruning: " + std::to_string(current_sequences.size()) + " sequences remain");
    
    // Limit the number of sequences to avoid explosion
    if (current_sequences.size() > 10000) {
        logger.debug("Limiting sequence count to 10000 (was " + std::to_string(current_sequences.size()) + ")");
        current_sequences.resize(10000);
    }
    
    logger.debug("Final sequence count for length " + std::to_string(length) + ": " + 
                std::to_string(current_sequences.size()));
}

void SequenceGenerator::generateSequencesRecursive(std::vector<uint8_t>& current, size_t remaining_bytes) {
    if (remaining_bytes == 0) {
        // We've built a complete sequence
        current_sequences.push_back(current);
        return;
    }
    
    // Try each opcode
    for (uint8_t opcode : valid_opcodes) {
        // Get info for this opcode
        InstructionInfo info = getInstructionInfo(opcode);
        
        // Check if we can fit this instruction
        if (info.bytes <= remaining_bytes) {
            // Add opcode
            current.push_back(opcode);
            
            // Generate all possible operands
            generateOperandCombinations(current, info, remaining_bytes - 1);
            
            // Remove opcode for next iteration
            current.pop_back();
        }
    }
}

void SequenceGenerator::generateOperandCombinations(std::vector<uint8_t>& current, 
                                                 const InstructionInfo& info,
                                                 size_t remaining_bytes) {
    // Number of operand bytes
    size_t operand_bytes = info.bytes - 1;
    
    if (operand_bytes == 0) {
        // No operands needed (implied instruction)
        generateSequencesRecursive(current, remaining_bytes);
        return;
    }
    
    // For instructions with operands, we'll use a simplified approach
    // to avoid combinatorial explosion. In a real implementation, this would
    // be more sophisticated.
    
    if (operand_bytes == 1) {
        // Single byte operand (like immediate or zero page)
        // Use a few representative values
        for (uint8_t operand : {0x00, 0x01, 0x20, 0x40, 0x80, 0xFF}) {
            current.push_back(operand);
            generateSequencesRecursive(current, remaining_bytes - 1);
            current.pop_back();
        }
    } else if (operand_bytes == 2) {
        // Two byte operand (like absolute)
        // Use a few representative addresses
        for (uint16_t operand : {0x0000, 0x0020, 0x0080, 0x0100, 0x2000, 0x3000}) {
            current.push_back(operand & 0xFF);
            current.push_back((operand >> 8) & 0xFF);
            generateSequencesRecursive(current, remaining_bytes - 2);
            current.pop_back();
            current.pop_back();
        }
    }
}

SequenceGenerator::InstructionInfo SequenceGenerator::getInstructionInfo(uint8_t opcode) {
    auto it = opcode_info.find(opcode);
    if (it != opcode_info.end()) {
        return it->second;
    }
    
    // Default for unknown opcodes
    return InstructionInfo(opcode, 1, 2, "???", "implied");
}

void SequenceGenerator::pruneInvalidSequences() {
    Logger& logger = getLogger();
    logger.debug("Pruning invalid sequences from " + std::to_string(current_sequences.size()) + " sequences");
    
    std::vector<std::vector<uint8_t>> valid_sequences;
    
    for (const auto& seq : current_sequences) {
        if (!shouldPrune(seq)) {
            valid_sequences.push_back(seq);
        }
    }
    
    logger.debug("Pruned " + std::to_string(current_sequences.size() - valid_sequences.size()) + " sequences");
    current_sequences = std::move(valid_sequences);
}

bool SequenceGenerator::shouldPrune(const std::vector<uint8_t>& sequence) {
    // Check for useless instruction sequences
    if (hasNoEffect(sequence)) return true;
    
    // Check for redundant instructions
    if (hasRedundantInstructions(sequence)) return true;
    
    // More pruning rules could be added here
    
    return false;
}

bool SequenceGenerator::hasNoEffect(const std::vector<uint8_t>& sequence) {
    // Simple check for NOP-only sequences
    return sequence.size() > 0 && 
           std::all_of(sequence.begin(), sequence.end(), 
                     [](uint8_t b) { return b == 0xEA; });
}

bool SequenceGenerator::hasRedundantInstructions(const std::vector<uint8_t>& sequence) {
    // This is a simplification. A real implementation would do more sophisticated
    // analysis of instruction patterns.
    
    // Example: LDA #x followed immediately by LDA #y
    for (size_t i = 0; i < sequence.size() - 2; i++) {
        if (sequence[i] == 0xA9 && sequence[i+2] == 0xA9) {
            return true;
        }
    }
    
    return false;
}

void SequenceGenerator::initOpcodeInfo() {
    // This is a simplified subset. A real implementation would include all opcodes.
    opcode_info = {
        {0xA9, {0xA9, 2, 2, "LDA", "immediate"}},
        {0x85, {0x85, 2, 3, "STA", "zeropage"}},
        {0x95, {0x95, 2, 4, "STA", "zeropage,X"}},
        {0x8D, {0x8D, 3, 4, "STA", "absolute"}},
        {0x9D, {0x9D, 3, 5, "STA", "absolute,X"}},
        {0x99, {0x99, 3, 5, "STA", "absolute,Y"}},
        {0xA2, {0xA2, 2, 2, "LDX", "immediate"}},
        {0xA0, {0xA0, 2, 2, "LDY", "immediate"}},
        {0x18, {0x18, 1, 2, "CLC", "implied"}},
        {0x38, {0x38, 1, 2, "SEC", "implied"}},
        {0xE8, {0xE8, 1, 2, "INX", "implied"}},
        {0xC8, {0xC8, 1, 2, "INY", "implied"}},
        {0xCA, {0xCA, 1, 2, "DEX", "implied"}},
        {0x88, {0x88, 1, 2, "DEY", "implied"}},
        {0xE6, {0xE6, 2, 5, "INC", "zeropage"}},
        {0xC6, {0xC6, 2, 5, "DEC", "zeropage"}},
        {0x69, {0x69, 2, 2, "ADC", "immediate"}},
        {0xC9, {0xC9, 2, 2, "CMP", "immediate"}},
        {0xD0, {0xD0, 2, 2, "BNE", "relative"}},
        {0xF0, {0xF0, 2, 2, "BEQ", "relative"}},
        {0x90, {0x90, 2, 2, "BCC", "relative"}},
        {0xB0, {0xB0, 2, 2, "BCS", "relative"}},
        {0x4C, {0x4C, 3, 3, "JMP", "absolute"}},
        {0xEA, {0xEA, 1, 2, "NOP", "implied"}},
        {0x00, {0x00, 1, 7, "BRK", "implied"}}
    };
}

} // namespace phaistos
