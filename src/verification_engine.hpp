/**
 * @file verification_engine.hpp
 * @brief Verification engine for 6502 code sequences
 */
#pragma once

#include "common.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "optimization_spec.hpp"

namespace phaistos {

/**
 * @class VerificationEngine
 * @brief Engine to verify if a sequence of instructions meets requirements
 */
template<typename AddressT = uint16_t>
class VerificationEngine {
public:
    /**
     * @brief Constructor
     * @param spec The optimization specification
     */
    VerificationEngine(const OptimizationSpec<AddressT>& spec);
    
    /**
     * @brief Check if a sequence is valid
     * @param sequence The instruction sequence to check
     * @return True if the sequence is valid
     */
    bool verify(const std::vector<uint8_t>& sequence);
    
    /**
     * @brief Check if a sequence is valid, with explanation of failure
     * @param sequence The instruction sequence to check
     * @param explanation Output parameter for explanation of failure
     * @return True if the sequence is valid
     */
    bool verifyWithExplanation(const std::vector<uint8_t>& sequence, 
                             std::string& explanation);
    
    /**
     * @brief Get the size of a sequence
     * @param sequence The instruction sequence
     * @return Size in bytes
     */
    size_t getSize(const std::vector<uint8_t>& sequence) const;
    
    /**
     * @brief Get the cycle count for a sequence
     * @param sequence The instruction sequence
     * @return Cycle count
     */
    size_t getCycles(const std::vector<uint8_t>& sequence) const;
    
private:
    /**
     * @brief Reference to the optimization specification
     */
    const OptimizationSpec<AddressT>& spec;
    
    /**
     * @struct TestCase
     * @brief Represents a test case with specific input values
     */
    struct TestCase {
        /** CPU state */
        typename CPU6502::State cpu;
        
        /** Memory values (only explicitly set values) */
        std::map<AddressT, uint8_t> memory_values;
    };
    
    /**
     * @brief Generate test cases from the specification
     * @return Vector of test cases
     */
    std::vector<TestCase> generateTestCases() const;
    
    /**
     * @brief Run a test case
     * @param sequence The instruction sequence
     * @param test The test case
     * @return True if the test passes
     */
    bool runTest(const std::vector<uint8_t>& sequence, const TestCase& test) const;
    
    /**
     * @brief Check if a CPU state matches requirements
     * @param actual The actual CPU state
     * @param test The test case (for SAME value checking)
     * @param expected_cpu The expected CPU state
     * @param expected_flags The expected flag state
     * @return True if the state matches
     */
    bool matchesCPUState(const typename CPU6502::State& actual,
                       const TestCase& test,
                       const typename OptimizationSpec<AddressT>::CPUState& expected_cpu,
                       const typename OptimizationSpec<AddressT>::FlagState& expected_flags) const;
    
    /**
     * @brief Check if memory state matches requirements
     * @param memory The memory to check
     * @param test The test case (for SAME value checking)
     * @param expected The expected memory regions
     * @return True if memory matches
     */
    bool matchesMemoryState(const TrackedMemory<AddressT>& memory,
                          const TestCase& test,
                          const std::vector<typename OptimizationSpec<AddressT>::MemoryRegion>& expected) const;
    
    /**
     * @brief Check for unauthorized memory modifications
     * @param memory The memory to check
     * @param allowed The allowed memory regions
     * @return True if there are unauthorized modifications
     */
    bool hasUnauthorizedModifications(const TrackedMemory<AddressT>& memory,
                                   const std::vector<typename OptimizationSpec<AddressT>::MemoryRegion>& allowed) const;
    
    /**
     * @brief Run a test case with detailed explanation
     * @param sequence The instruction sequence
     * @param test The test case
     * @param explanation Output parameter for explanation of failure
     * @return True if the test passes
     */
    bool runTestWithExplanation(const std::vector<uint8_t>& sequence, 
                              const TestCase& test,
                              std::string& explanation) const;
};

} // namespace phaistos
