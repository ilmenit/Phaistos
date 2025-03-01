/**
 * @file sequence_generator.hpp
 * @brief Sequence generator for instruction sequences
 */
#pragma once

#include "common.hpp"
#include "cpu.hpp"

namespace phaistos {

/**
 * @class SequenceGenerator
 * @brief Generates candidate instruction sequences
 */
class SequenceGenerator {
public:
    /**
     * @brief Constructor
     */
    SequenceGenerator();
    
    /**
     * @brief Set the maximum sequence length
     * @param max_length The maximum length to generate
     */
    void setMaxLength(size_t max_length);
    
    /**
     * @brief Set the valid opcodes to use
     * @param opcodes Vector of valid opcodes
     */
    void setValidOpcodes(const std::vector<uint8_t>& opcodes);
    
    /**
     * @brief Reset the generator to initial state
     */
    void reset();
    
    /**
     * @brief Get the next candidate sequence
     * @param sequence Output parameter for the next sequence
     * @return True if a sequence was generated, false if done
     */
    bool next(std::vector<uint8_t>& sequence);
    
private:
    /**
     * @struct InstructionInfo
     * @brief Information about a 6502 instruction
     */
    struct InstructionInfo {
        uint8_t opcode;
        uint8_t bytes;
        uint8_t cycles;
        std::string mnemonic;
        std::string addressing_mode;
        
        InstructionInfo() : opcode(0), bytes(0), cycles(0) {}
        
        InstructionInfo(uint8_t op, uint8_t b, uint8_t c, 
                      const std::string& m, const std::string& a)
            : opcode(op), bytes(b), cycles(c), mnemonic(m), addressing_mode(a) {}
    };
    
    // Generator state
    size_t current_length;
    size_t max_length;
    std::vector<uint8_t> valid_opcodes;
    size_t current_combination_index;
    std::vector<std::vector<uint8_t>> current_sequences;
    
    // Opcode information
    std::unordered_map<uint8_t, InstructionInfo> opcode_info;
    
    /**
     * @brief Generate sequences of a specific length
     * @param length Length to generate
     */
    void generateSequencesOfLength(size_t length);
    
    /**
     * @brief Recursive helper for sequence generation
     * @param current Current sequence being built
     * @param remaining_bytes Remaining bytes to add
     */
    void generateSequencesRecursive(std::vector<uint8_t>& current, size_t remaining_bytes);
    
    /**
     * @brief Generate all operand combinations for an instruction
     * @param current Current sequence
     * @param info Instruction info
     * @param remaining_bytes Remaining bytes
     */
    void generateOperandCombinations(std::vector<uint8_t>& current, 
                                   const InstructionInfo& info,
                                   size_t remaining_bytes);
    
    /**
     * @brief Get information about an opcode
     * @param opcode Opcode to look up
     * @return Instruction information
     */
    InstructionInfo getInstructionInfo(uint8_t opcode);
    
    /**
     * @brief Prune obviously invalid sequences
     */
    void pruneInvalidSequences();
    
    /**
     * @brief Check if a sequence should be pruned
     * @param sequence Sequence to check
     * @return True if the sequence should be pruned
     */
    bool shouldPrune(const std::vector<uint8_t>& sequence);
    
    /**
     * @brief Check if a sequence has no effect
     * @param sequence Sequence to check
     * @return True if the sequence has no effect
     */
    bool hasNoEffect(const std::vector<uint8_t>& sequence);
    
    /**
     * @brief Check if a sequence has redundant instructions
     * @param sequence Sequence to check
     * @return True if the sequence has redundant instructions
     */
    bool hasRedundantInstructions(const std::vector<uint8_t>& sequence);
    
    /**
     * @brief Initialize opcode information
     */
    void initOpcodeInfo();
};

} // namespace phaistos
