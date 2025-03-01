/**
 * @file cpu.hpp
 * @brief CPU interface and 6502 implementation
 */
#pragma once

#include "common.hpp"
#include "memory.hpp"

namespace phaistos {

/**
 * @struct ExecutionResult
 * @brief Represents the result of executing a sequence of instructions
 */
struct ExecutionResult {
    size_t cycles;              // Cycles consumed
    size_t instructions;        // Instructions executed
    bool completed;             // True if execution completed normally
    
    enum ErrorType {
        NONE,
        INVALID_INSTRUCTION,
        MEMORY_ACCESS_VIOLATION,
        EXECUTION_LIMIT_REACHED,
        OTHER
    } error;
    
    std::string error_message;  // Details about any error
    
    // Constructor with sensible defaults
    ExecutionResult() 
        : cycles(0), instructions(0), completed(false), 
          error(NONE), error_message("") {}
};

/**
 * @class CPU
 * @brief Generic CPU interface
 */
template<typename AddressT = uint16_t>
class CPU {
public:
    virtual ~CPU() = default;
    
    /**
     * @brief Execute instructions from a memory location
     * @param memory Memory to execute from
     * @param start_address Address to start execution at
     * @param max_instructions Maximum number of instructions to execute
     * @return Result of execution
     */
    virtual ExecutionResult execute(
        Memory<AddressT>& memory, 
        AddressT start_address, 
        size_t max_instructions = SIZE_MAX
    ) = 0;
    
    /**
     * @brief Factory method to create CPU instance by architecture
     * @param architecture Architecture name (e.g., "6502")
     * @return Unique pointer to CPU instance
     */
    static std::unique_ptr<CPU<AddressT>> create(const std::string& architecture);
};

/**
 * @class CPU6502
 * @brief 6502 CPU implementation
 */
class CPU6502 : public CPU<uint16_t> {
public:
    /**
     * @struct State
     * @brief 6502 CPU state
     */
    struct State {
        uint8_t a, x, y, sp;
        bool c, z, i, d, b, v, n;
        uint16_t pc;
        
        // Constructor with defaults
        State() : a(0), x(0), y(0), sp(0xFF), 
                 c(false), z(false), i(false), d(false), b(false), v(false), n(false),
                 pc(0) {}
    };
    
    /**
     * @brief Set the CPU state
     * @param newState The state to set
     */
    void setState(const State& newState) {
        this->state = newState;
    }
    
    /**
     * @brief Get the current CPU state
     * @return Current state
     */
    State getState() const {
        return state;
    }
    
    /**
     * @brief Execute instructions from memory
     * @param memory Memory to execute from
     * @param start_address Address to start execution
     * @param max_instructions Maximum instructions to execute
     * @return Result of execution
     */
    ExecutionResult execute(
        Memory<uint16_t>& memory,
        uint16_t start_address, 
        size_t max_instructions = SIZE_MAX
    ) override;
    
private:
    State state;
    
    /**
     * @brief Execute a single instruction
     * @param memory Memory to execute from
     * @param execResult Result structure to update
     */
    void executeInstruction(Memory<uint16_t>& memory, ExecutionResult& execResult);
    
    /**
     * @brief Update Zero and Negative flags based on a value
     * @param value Value to check
     */
    void updateZN(uint8_t value) {
        state.z = (value == 0);
        state.n = (value & 0x80) != 0;
    }
    
    /**
     * @brief Get cycle count for an instruction
     * @param opcode Opcode to check
     * @param page_cross Whether a page boundary was crossed
     * @return Number of cycles
     */
    size_t getInstructionCycles(uint8_t opcode, bool page_cross = false);
    
    /**
     * @brief Get size of an instruction in bytes
     * @param opcode Opcode to check
     * @return Size in bytes
     */
    size_t getInstructionSize(uint8_t opcode);
};

/**
 * @brief Get a vector of all valid 6502 opcodes
 * @return Vector of valid opcodes
 */
std::vector<uint8_t> getAllValidOpcodes();

} // namespace phaistos
