/**
 * @file optimization_spec.hpp
 * @brief Defines the optimization specification data structure
 */
#pragma once

#include "common.hpp"
#include "value.hpp"

namespace phaistos {

/**
 * @class OptimizationSpec
 * @brief Represents a complete optimization specification
 */
template<typename AddressT = uint16_t>
class OptimizationSpec {
public:
    /**
     * @enum OptimizationGoal
     * @brief Goal for optimization (size or speed)
     */
    enum OptimizationGoal { SIZE, SPEED };

    /**
     * @struct CPUState
     * @brief CPU register state (A, X, Y, SP)
     */
    struct CPUState {
        Value a, x, y, sp;
    };

    /**
     * @struct FlagState
     * @brief CPU flag state (C, Z, I, D, B, V, N)
     */
    struct FlagState {
        Value c, z, i, d, b, v, n;
    };

    /**
     * @struct MemoryRegion
     * @brief Region of memory with values
     */
    struct MemoryRegion {
        AddressT address;
        std::vector<Value> bytes;
        
        /**
         * @brief Check if an address is within this region
         * @param addr The address to check
         * @return True if the address is contained in this region
         */
        bool containsAddress(AddressT addr) const {
            return addr >= address && addr < address + bytes.size();
        }
        
        /**
         * @brief Check if an address requires an exact value
         * @param addr The address to check
         * @return True if the address is in this region and requires an exact value
         */
        bool requiresExactValue(AddressT addr) const {
            if (!containsAddress(addr)) return false;
            return bytes[addr - address].type == Value::EXACT;
        }
    };

    /**
     * @struct CodeBlock
     * @brief Block of code to be optimized
     */
    struct CodeBlock {
        enum Type { REGULAR, READ_ONLY };
        
        AddressT address;
        std::vector<uint8_t> bytes;
        Type type;
        
        CodeBlock() : address(0), type(REGULAR) {}
        
        CodeBlock(AddressT addr, const std::vector<uint8_t>& b, Type t)
            : address(addr), bytes(b), type(t) {}
    };
    
    OptimizationGoal goal = SIZE;
    AddressT run_address = 0;
    
    // Input and output states
    CPUState input_cpu;
    FlagState input_flags;
    std::vector<MemoryRegion> input_memory;
    
    CPUState output_cpu;
    FlagState output_flags;
    std::vector<MemoryRegion> output_memory;
    
    // Code blocks to optimize
    std::vector<CodeBlock> code_blocks;
};

} // namespace phaistos
