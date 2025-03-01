/**
 * @file cpu.cpp
 * @brief Implementation of CPU classes
 */
#include "cpu.hpp"

namespace phaistos {

template<typename AddressT>
std::unique_ptr<CPU<AddressT>> CPU<AddressT>::create(const std::string& architecture) {
    if (architecture == "6502") {
        return std::make_unique<CPU6502>();
    }
    // Add support for other CPUs here in the future
    
    throw std::runtime_error("Unsupported CPU architecture: " + architecture);
}

// Explicit instantiation for uint16_t
template std::unique_ptr<CPU<uint16_t>> CPU<uint16_t>::create(const std::string&);

ExecutionResult CPU6502::execute(
    Memory<uint16_t>& memory,
    uint16_t start_address, 
    size_t max_instructions
) {
    ExecutionResult execResult;
    
    // Set program counter to start address
    state.pc = start_address;
    
    // Execute instructions until done or limit reached
    while (execResult.instructions < max_instructions) {
        try {
            // Execute one instruction
            executeInstruction(memory, execResult);
            execResult.instructions++;
        } catch (const std::exception& e) {
            // Handle execution error
            execResult.error = ExecutionResult::INVALID_INSTRUCTION;
            execResult.error_message = e.what();
            return execResult;
        }
    }
    
    // Check if we hit the instruction limit
    if (execResult.instructions >= max_instructions) {
        execResult.error = ExecutionResult::EXECUTION_LIMIT_REACHED;
        execResult.error_message = "Maximum instruction count reached";
    } else {
        execResult.completed = true;
    }
    
    return execResult;
}

void CPU6502::executeInstruction(Memory<uint16_t>& memory, ExecutionResult& execResult) {
    // Read opcode
    uint8_t opcode = memory.read(state.pc++);
    
    // Track cycles (simplified - would be more accurate in a full implementation)
    execResult.cycles += getInstructionCycles(opcode);
    
    // Execute instruction based on opcode
    switch (opcode) {
        case 0xA9: // LDA immediate
            state.a = memory.read(state.pc++);
            updateZN(state.a);
            break;
            
        case 0x85: // STA zero page
            {
                uint8_t zpAddr = memory.read(state.pc++);
                memory.write(zpAddr, state.a);
            }
            break;
            
        case 0x95: // STA zero page,X
            {
                uint8_t zpAddr = memory.read(state.pc++);
                memory.write((zpAddr + state.x) & 0xFF, state.a);
            }
            break;
            
        case 0x8D: // STA absolute
            {
                uint16_t addr = memory.read16(state.pc);
                state.pc += 2;
                memory.write(addr, state.a);
            }
            break;
            
        case 0x9D: // STA absolute,X
            {
                uint16_t addr = memory.read16(state.pc);
                state.pc += 2;
                memory.write(addr + state.x, state.a);
            }
            break;
            
        case 0x99: // STA absolute,Y
            {
                uint16_t addr = memory.read16(state.pc);
                state.pc += 2;
                memory.write(addr + state.y, state.a);
            }
            break;
            
        case 0xA2: // LDX immediate
            state.x = memory.read(state.pc++);
            updateZN(state.x);
            break;
            
        case 0xA0: // LDY immediate
            state.y = memory.read(state.pc++);
            updateZN(state.y);
            break;
            
        case 0x18: // CLC
            state.c = false;
            break;
            
        case 0x38: // SEC
            state.c = true;
            break;
            
        case 0xE8: // INX
            state.x++;
            updateZN(state.x);
            break;
            
        case 0xC8: // INY
            state.y++;
            updateZN(state.y);
            break;
            
        case 0xCA: // DEX
            state.x--;
            updateZN(state.x);
            break;
            
        case 0x88: // DEY
            state.y--;
            updateZN(state.y);
            break;
            
        case 0xE6: // INC zero page
            {
                uint8_t zpAddr = memory.read(state.pc++);
                uint8_t val = memory.read(zpAddr);
                val++;
                memory.write(zpAddr, val);
                updateZN(val);
            }
            break;
            
        case 0xC6: // DEC zero page
            {
                uint8_t zpAddr = memory.read(state.pc++);
                uint8_t val = memory.read(zpAddr);
                val--;
                memory.write(zpAddr, val);
                updateZN(val);
            }
            break;
            
        case 0x69: // ADC immediate
            {
                uint8_t val = memory.read(state.pc++);
                uint16_t sum = state.a + val + (state.c ? 1 : 0);
                
                state.c = (sum > 0xFF);
                state.v = ((state.a ^ sum) & (val ^ sum) & 0x80) != 0;
                state.a = sum & 0xFF;
                updateZN(state.a);
            }
            break;
            
        case 0xC9: // CMP immediate
            {
                uint8_t val = memory.read(state.pc++);
                uint16_t diff = state.a - val;
                
                state.c = (state.a >= val);
                state.z = (state.a == val);
                state.n = ((diff & 0x80) != 0);
            }
            break;
            
        case 0xD0: // BNE
            {
                int8_t offset = static_cast<int8_t>(memory.read(state.pc++));
                if (!state.z) {
                    execResult.cycles += 1; // Branch taken
                    state.pc += offset;
                }
            }
            break;
            
        case 0xF0: // BEQ
            {
                int8_t offset = static_cast<int8_t>(memory.read(state.pc++));
                if (state.z) {
                    execResult.cycles += 1; // Branch taken
                    state.pc += offset;
                }
            }
            break;
            
        case 0x90: // BCC
            {
                int8_t offset = static_cast<int8_t>(memory.read(state.pc++));
                if (!state.c) {
                    execResult.cycles += 1; // Branch taken
                    state.pc += offset;
                }
            }
            break;
            
        case 0xB0: // BCS
            {
                int8_t offset = static_cast<int8_t>(memory.read(state.pc++));
                if (state.c) {
                    execResult.cycles += 1; // Branch taken
                    state.pc += offset;
                }
            }
            break;
            
        case 0x4C: // JMP absolute
            {
                uint16_t addr = memory.read16(state.pc);
                state.pc = addr;
            }
            break;
            
        case 0xEA: // NOP
            // Do nothing
            break;
            
        case 0x00: // BRK (simplified)
            execResult.completed = true;
            break;
            
        default:
            throw std::runtime_error("Unimplemented opcode: 0x" + 
                                    std::to_string(opcode));
    }
}

size_t CPU6502::getInstructionCycles(uint8_t opcode, bool page_cross) {
    // Simplified cycle count table - a real implementation would be more complete
    static const std::unordered_map<uint8_t, uint8_t> cycleTable = {
        {0xA9, 2}, // LDA immediate
        {0x85, 3}, // STA zero page
        {0x95, 4}, // STA zero page,X
        {0x8D, 4}, // STA absolute
        {0x9D, 5}, // STA absolute,X
        {0x99, 5}, // STA absolute,Y
        {0xA2, 2}, // LDX immediate
        {0xA0, 2}, // LDY immediate
        {0x18, 2}, // CLC
        {0x38, 2}, // SEC
        {0xE8, 2}, // INX
        {0xC8, 2}, // INY
        {0xCA, 2}, // DEX
        {0x88, 2}, // DEY
        {0xE6, 5}, // INC zero page
        {0xC6, 5}, // DEC zero page
        {0x69, 2}, // ADC immediate
        {0xC9, 2}, // CMP immediate
        {0xD0, 2}, // BNE (not taken)
        {0xF0, 2}, // BEQ (not taken)
        {0x90, 2}, // BCC (not taken)
        {0xB0, 2}, // BCS (not taken)
        {0x4C, 3}, // JMP absolute
        {0xEA, 2}, // NOP
        {0x00, 7}  // BRK
    };
    
    auto it = cycleTable.find(opcode);
    if (it != cycleTable.end()) {
        return it->second + (page_cross ? 1 : 0);
    }
    
    // Default for unknown opcodes
    return 2;
}

size_t CPU6502::getInstructionSize(uint8_t opcode) {
    // Simplified instruction size table - a real implementation would be more complete
    static const std::unordered_map<uint8_t, uint8_t> sizeTable = {
        {0xA9, 2}, // LDA immediate
        {0x85, 2}, // STA zero page
        {0x95, 2}, // STA zero page,X
        {0x8D, 3}, // STA absolute
        {0x9D, 3}, // STA absolute,X
        {0x99, 3}, // STA absolute,Y
        {0xA2, 2}, // LDX immediate
        {0xA0, 2}, // LDY immediate
        {0x18, 1}, // CLC
        {0x38, 1}, // SEC
        {0xE8, 1}, // INX
        {0xC8, 1}, // INY
        {0xCA, 1}, // DEX
        {0x88, 1}, // DEY
        {0xE6, 2}, // INC zero page
        {0xC6, 2}, // DEC zero page
        {0x69, 2}, // ADC immediate
        {0xC9, 2}, // CMP immediate
        {0xD0, 2}, // BNE
        {0xF0, 2}, // BEQ
        {0x90, 2}, // BCC
        {0xB0, 2}, // BCS
        {0x4C, 3}, // JMP absolute
        {0xEA, 1}, // NOP
        {0x00, 1}  // BRK
    };
    
    auto it = sizeTable.find(opcode);
    if (it != sizeTable.end()) {
        return it->second;
    }
    
    // Default for unknown opcodes
    return 1;
}

std::vector<uint8_t> getAllValidOpcodes() {
    // A subset of valid 6502 opcodes - a real implementation would include all
    return {
        0xA9, 0x85, 0x95, 0x8D, 0x9D, 0x99, // LDA, STA
        0xA2, 0xA0, // LDX, LDY
        0x18, 0x38, // CLC, SEC
        0xE8, 0xC8, 0xCA, 0x88, // INX, INY, DEX, DEY
        0xE6, 0xC6, // INC, DEC
        0x69, 0xC9, // ADC, CMP
        0xD0, 0xF0, 0x90, 0xB0, // BNE, BEQ, BCC, BCS
        0x4C, // JMP
        0xEA, // NOP
        0x00  // BRK
    };
}

} // namespace phaistos
