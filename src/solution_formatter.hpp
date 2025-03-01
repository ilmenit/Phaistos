/**
 * @file solution_formatter.hpp
 * @brief Formatter for optimizer output
 */
#pragma once

#include "common.hpp"

namespace phaistos {

/**
 * @class SolutionFormatter
 * @brief Formats optimization solutions in various formats
 */
class SolutionFormatter {
public:
    /**
     * @brief Constructor
     */
    SolutionFormatter();
    
    /**
     * @enum Format
     * @brief Output formats
     */
    enum Format {
        ASSEMBLY,     ///< 6502 assembly language
        BINARY,       ///< Raw binary bytes
        C_ARRAY,      ///< C/C++ byte array
        BASIC_DATA    ///< BASIC DATA statements
    };
    
    /**
     * @brief Format a solution
     * @param solution Instruction sequence
     * @param format Output format
     * @return Formatted solution
     */
    std::string format(const std::vector<uint8_t>& solution, 
                     Format format = ASSEMBLY);
    
    /**
     * @brief Get statistics about a solution
     * @param solution Instruction sequence
     * @return String containing statistics
     */
    std::string getStatistics(const std::vector<uint8_t>& solution);
    
private:
    /**
     * @struct InstructionInfo
     * @brief Information about a 6502 instruction
     */
    struct InstructionInfo {
        std::string mnemonic;
        uint8_t bytes;
        uint8_t cycles;
        std::string addressing_mode;
        
        InstructionInfo(const std::string& m = "???", uint8_t b = 1, uint8_t c = 2, 
                      const std::string& a = "implied")
            : mnemonic(m), bytes(b), cycles(c), addressing_mode(a) {}
    };
    
    /** Information about all opcodes */
    std::unordered_map<uint8_t, InstructionInfo> opcode_info;
    
    /**
     * @brief Disassemble bytes to 6502 assembly
     * @param bytes Instruction bytes
     * @return Assembly code
     */
    std::string disassemble(const std::vector<uint8_t>& bytes);
    
    /**
     * @brief Format as raw bytes (hex dump)
     * @param bytes Instruction bytes
     * @return Hex dump
     */
    std::string formatBytes(const std::vector<uint8_t>& bytes);
    
    /**
     * @brief Format as C/C++ array
     * @param bytes Instruction bytes
     * @return C array
     */
    std::string formatCArray(const std::vector<uint8_t>& bytes);
    
    /**
     * @brief Format as BASIC DATA statements
     * @param bytes Instruction bytes
     * @return BASIC code
     */
    std::string formatBasicData(const std::vector<uint8_t>& bytes);
    
    /**
     * @brief Get info for an opcode
     * @param opcode Opcode to look up
     * @return Instruction information
     */
    InstructionInfo getInstructionInfo(uint8_t opcode);
    
    /**
     * @brief Calculate size of a solution
     * @param solution Instruction sequence
     * @return Size in bytes
     */
    size_t calculateSize(const std::vector<uint8_t>& solution) const;
    
    /**
     * @brief Calculate cycles for a solution
     * @param solution Instruction sequence
     * @return Cycle count
     */
    size_t calculateCycles(const std::vector<uint8_t>& solution) const;
    
    /**
     * @brief Initialize opcode information
     */
    void initOpcodeInfo();
};

} // namespace phaistos
