/**
 * @file solution_formatter.cpp
 * @brief Implementation of the solution formatter
 */
#include "solution_formatter.hpp"

namespace phaistos {

SolutionFormatter::SolutionFormatter() {
    initOpcodeInfo();
}

std::string SolutionFormatter::format(const std::vector<uint8_t>& solution, 
                                   Format format) {
    switch (format) {
        case ASSEMBLY:
            return disassemble(solution);
        case BINARY:
            return formatBytes(solution);
        case C_ARRAY:
            return formatCArray(solution);
        case BASIC_DATA:
            return formatBasicData(solution);
        default:
            return disassemble(solution);
    }
}

std::string SolutionFormatter::getStatistics(const std::vector<uint8_t>& solution) {
    size_t size = calculateSize(solution);
    size_t cycles = calculateCycles(solution);
    
    std::ostringstream oss;
    oss << "Size: " << size << " bytes\n";
    oss << "Cycles: " << cycles << " (approximate)\n";
    
    return oss.str();
}

std::string SolutionFormatter::disassemble(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    
    // Track position and add comments about size/cycles
    size_t pos = 0;
    while (pos < bytes.size()) {
        uint8_t opcode = bytes[pos++];
        
        // Get instruction info
        auto info = getInstructionInfo(opcode);
        
        // Start line with whitespace
        oss << "        ";
        
        // Add mnemonic
        oss << info.mnemonic << " ";
        
        // Add operands based on addressing mode
        size_t operand_bytes = info.bytes - 1;
        if (operand_bytes > 0 && pos < bytes.size()) {
            if (info.addressing_mode == "immediate") {
                oss << "#$" << std::hex << std::uppercase 
                    << std::setw(2) << std::setfill('0')
                    << static_cast<int>(bytes[pos++]);
            } else if (info.addressing_mode == "zeropage") {
                oss << "$" << std::hex << std::uppercase 
                    << std::setw(2) << std::setfill('0')
                    << static_cast<int>(bytes[pos++]);
            } else if (info.addressing_mode == "zeropage,X") {
                oss << "$" << std::hex << std::uppercase 
                    << std::setw(2) << std::setfill('0')
                    << static_cast<int>(bytes[pos++]) << ",X";
            } else if (info.addressing_mode == "zeropage,Y") {
                oss << "$" << std::hex << std::uppercase 
                    << std::setw(2) << std::setfill('0')
                    << static_cast<int>(bytes[pos++]) << ",Y";
            } else if (info.addressing_mode == "absolute" && pos + 1 <= bytes.size()) {
                uint16_t addr = bytes[pos++];
                if (pos < bytes.size()) {
                    addr |= (static_cast<uint16_t>(bytes[pos++]) << 8);
                }
                oss << "$" << std::hex << std::uppercase 
                    << std::setw(4) << std::setfill('0') << addr;
            } else if (info.addressing_mode == "absolute,X" && pos + 1 <= bytes.size()) {
                uint16_t addr = bytes[pos++];
                if (pos < bytes.size()) {
                    addr |= (static_cast<uint16_t>(bytes[pos++]) << 8);
                }
                oss << "$" << std::hex << std::uppercase 
                    << std::setw(4) << std::setfill('0') << addr << ",X";
            } else if (info.addressing_mode == "absolute,Y" && pos + 1 <= bytes.size()) {
                uint16_t addr = bytes[pos++];
                if (pos < bytes.size()) {
                    addr |= (static_cast<uint16_t>(bytes[pos++]) << 8);
                }
                oss << "$" << std::hex << std::uppercase 
                    << std::setw(4) << std::setfill('0') << addr << ",Y";
            } else if (info.addressing_mode == "relative") {
                // Handle relative addresses (branches)
                int8_t offset = static_cast<int8_t>(bytes[pos++]);
                uint16_t target = pos + offset;
                oss << "$" << std::hex << std::uppercase 
                    << std::setw(4) << std::setfill('0') << target;
            } else {
                // Unknown addressing mode, just show raw bytes
                while (operand_bytes > 0 && pos < bytes.size()) {
                    oss << "$" << std::hex << std::uppercase 
                        << std::setw(2) << std::setfill('0')
                        << static_cast<int>(bytes[pos++]) << " ";
                    operand_bytes--;
                }
            }
        }
        
        // Add comment with size and cycles
        oss << "    ; " << std::dec << info.bytes << " bytes, " 
            << info.cycles << " cycles";
        
        oss << "\n";
    }
    
    return oss.str();
}

std::string SolutionFormatter::formatBytes(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    
    for (size_t i = 0; i < bytes.size(); i++) {
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(bytes[i]);
        
        if (i < bytes.size() - 1) {
            oss << " ";
        }
        
        // Add newline every 16 bytes
        if ((i + 1) % 16 == 0 && i < bytes.size() - 1) {
            oss << "\n";
        }
    }
    
    return oss.str();
}

std::string SolutionFormatter::formatCArray(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    
    oss << "// Generated by Phaistos 6502 Optimizer\n";
    oss << "const unsigned char optimized_code[" << bytes.size() << "] = {\n    ";
    
    for (size_t i = 0; i < bytes.size(); i++) {
        oss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(bytes[i]);
        
        if (i < bytes.size() - 1) {
            oss << ", ";
        }
        
        // Add newline every 8 bytes
        if ((i + 1) % 8 == 0 && i < bytes.size() - 1) {
            oss << "\n    ";
        }
    }
    
    oss << "\n};\n";
    
    return oss.str();
}

std::string SolutionFormatter::formatBasicData(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    
    oss << "10 REM GENERATED BY PHAISTOS 6502 OPTIMIZER\n";
    
    size_t line_num = 100;
    oss << line_num << " DATA ";
    
    for (size_t i = 0; i < bytes.size(); i++) {
        oss << static_cast<int>(bytes[i]);
        
        if (i < bytes.size() - 1) {
            oss << ",";
        }
        
        // Add new line every 8 values
        if ((i + 1) % 8 == 0 && i < bytes.size() - 1) {
            line_num += 10;
            oss << "\n" << line_num << " DATA ";
        }
    }
    
    oss << "\n";
    
    return oss.str();
}

SolutionFormatter::InstructionInfo SolutionFormatter::getInstructionInfo(uint8_t opcode) {
    auto it = opcode_info.find(opcode);
    if (it != opcode_info.end()) {
        return it->second;
    }
    
    // Default for unknown opcodes
    return InstructionInfo("???", 1, 2, "implied");
}

size_t SolutionFormatter::calculateSize(const std::vector<uint8_t>& solution) const {
    return solution.size();
}

size_t SolutionFormatter::calculateCycles(const std::vector<uint8_t>& solution) const {
    size_t total_cycles = 0;
    size_t pos = 0;
    
    while (pos < solution.size()) {
        uint8_t opcode = solution[pos++];
        auto info_it = opcode_info.find(opcode);
        
        if (info_it != opcode_info.end()) {
            total_cycles += info_it->second.cycles;
            pos += info_it->second.bytes - 1; // Skip operand bytes
        } else {
            // Unknown opcode, assume 2 cycles and 1 byte
            total_cycles += 2;
        }
        
        if (pos > solution.size()) {
            // Incomplete instruction at the end
            break;
        }
    }
    
    return total_cycles;
}

void SolutionFormatter::initOpcodeInfo() {
    // This is a simplified subset. A real implementation would include all opcodes.
    opcode_info = {
        {0xA9, {"LDA", 2, 2, "immediate"}},
        {0xA5, {"LDA", 2, 3, "zeropage"}},
        {0xB5, {"LDA", 2, 4, "zeropage,X"}},
        {0xAD, {"LDA", 3, 4, "absolute"}},
        {0xBD, {"LDA", 3, 4, "absolute,X"}},
        {0xB9, {"LDA", 3, 4, "absolute,Y"}},
        
        {0x85, {"STA", 2, 3, "zeropage"}},
        {0x95, {"STA", 2, 4, "zeropage,X"}},
        {0x8D, {"STA", 3, 4, "absolute"}},
        {0x9D, {"STA", 3, 5, "absolute,X"}},
        {0x99, {"STA", 3, 5, "absolute,Y"}},
        
        {0xA2, {"LDX", 2, 2, "immediate"}},
        {0xA0, {"LDY", 2, 2, "immediate"}},
        
        {0x18, {"CLC", 1, 2, "implied"}},
        {0x38, {"SEC", 1, 2, "implied"}},
        {0xE8, {"INX", 1, 2, "implied"}},
        {0xC8, {"INY", 1, 2, "implied"}},
        {0xCA, {"DEX", 1, 2, "implied"}},
        {0x88, {"DEY", 1, 2, "implied"}},
        
        {0xE6, {"INC", 2, 5, "zeropage"}},
        {0xF6, {"INC", 2, 6, "zeropage,X"}},
        {0xEE, {"INC", 3, 6, "absolute"}},
        {0xFE, {"INC", 3, 7, "absolute,X"}},
        
        {0xC6, {"DEC", 2, 5, "zeropage"}},
        {0xD6, {"DEC", 2, 6, "zeropage,X"}},
        {0xCE, {"DEC", 3, 6, "absolute"}},
        {0xDE, {"DEC", 3, 7, "absolute,X"}},
        
        {0x69, {"ADC", 2, 2, "immediate"}},
        {0x65, {"ADC", 2, 3, "zeropage"}},
        {0x75, {"ADC", 2, 4, "zeropage,X"}},
        {0x6D, {"ADC", 3, 4, "absolute"}},
        {0x7D, {"ADC", 3, 4, "absolute,X"}},
        {0x79, {"ADC", 3, 4, "absolute,Y"}},
        
        {0xC9, {"CMP", 2, 2, "immediate"}},
        {0xC5, {"CMP", 2, 3, "zeropage"}},
        {0xD5, {"CMP", 2, 4, "zeropage,X"}},
        {0xCD, {"CMP", 3, 4, "absolute"}},
        {0xDD, {"CMP", 3, 4, "absolute,X"}},
        {0xD9, {"CMP", 3, 4, "absolute,Y"}},
        
        {0xD0, {"BNE", 2, 2, "relative"}},
        {0xF0, {"BEQ", 2, 2, "relative"}},
        {0x90, {"BCC", 2, 2, "relative"}},
        {0xB0, {"BCS", 2, 2, "relative"}},
        {0x10, {"BPL", 2, 2, "relative"}},
        {0x30, {"BMI", 2, 2, "relative"}},
        {0x50, {"BVC", 2, 2, "relative"}},
        {0x70, {"BVS", 2, 2, "relative"}},
        
        {0x4C, {"JMP", 3, 3, "absolute"}},
        {0x6C, {"JMP", 3, 5, "indirect"}},
        
        {0x20, {"JSR", 3, 6, "absolute"}},
        {0x60, {"RTS", 1, 6, "implied"}},
        
        {0xEA, {"NOP", 1, 2, "implied"}},
        {0x00, {"BRK", 1, 7, "implied"}}
    };
}

} // namespace phaistos
