/**
 * @file verification_engine.cpp
 * @brief Implementation of the verification engine
 */
#include "verification_engine.hpp"

namespace phaistos {

template<typename AddressT>
VerificationEngine<AddressT>::VerificationEngine(const OptimizationSpec<AddressT>& specParam)
    : spec(specParam) {
}

template<typename AddressT>
bool VerificationEngine<AddressT>::verify(const std::vector<uint8_t>& sequence) {
    // Generate test cases from the specification
    std::vector<TestCase> test_cases = generateTestCases();
    
    // Run each test case
    for (const auto& test_case : test_cases) {
        if (!runTest(sequence, test_case)) {
            return false; // Failed a test case
        }
    }
    
    // All test cases passed
    return true;
}

template<typename AddressT>
bool VerificationEngine<AddressT>::verifyWithExplanation(
    const std::vector<uint8_t>& sequence, 
    std::string& explanation) {
    
    std::vector<TestCase> test_cases = generateTestCases();
    
    for (const auto& test_case : test_cases) {
        std::string failure_reason;
        if (!runTestWithExplanation(sequence, test_case, failure_reason)) {
            explanation = failure_reason;
            return false;
        }
    }
    
    return true;
}

template<typename AddressT>
size_t VerificationEngine<AddressT>::getSize(const std::vector<uint8_t>& sequence) const {
    return sequence.size();
}

template<typename AddressT>
size_t VerificationEngine<AddressT>::getCycles(const std::vector<uint8_t>& sequence) const {
    // Create CPU and memory for cycle counting
    CPU6502 cpu;
    TrackedMemory<AddressT> memory;
    
    // Set up minimal state
    typename CPU6502::State state;
    state.pc = 0x1000; // Arbitrary address
    cpu.setState(state);
    
    // Copy sequence to memory
    for (size_t i = 0; i < sequence.size(); i++) {
        memory.initialize(static_cast<AddressT>(0x1000 + i), sequence[i]);
    }
    
    // Execute and get cycle count
    ExecutionResult result = cpu.execute(memory, 0x1000);
    return result.cycles;
}

template<typename AddressT>
std::vector<typename VerificationEngine<AddressT>::TestCase> 
VerificationEngine<AddressT>::generateTestCases() const {
    std::vector<TestCase> test_cases;
    
    // Start with base test case
    TestCase base;
    
    // Set EXACT values for CPU registers
    if (spec.input_cpu.a.type == Value::EXACT) base.cpu.a = spec.input_cpu.a.exact_value;
    if (spec.input_cpu.x.type == Value::EXACT) base.cpu.x = spec.input_cpu.x.exact_value;
    if (spec.input_cpu.y.type == Value::EXACT) base.cpu.y = spec.input_cpu.y.exact_value;
    if (spec.input_cpu.sp.type == Value::EXACT) base.cpu.sp = spec.input_cpu.sp.exact_value;
    
    // Set EXACT values for flags
    if (spec.input_flags.c.type == Value::EXACT) base.cpu.c = spec.input_flags.c.exact_value;
    if (spec.input_flags.z.type == Value::EXACT) base.cpu.z = spec.input_flags.z.exact_value;
    if (spec.input_flags.i.type == Value::EXACT) base.cpu.i = spec.input_flags.i.exact_value;
    if (spec.input_flags.d.type == Value::EXACT) base.cpu.d = spec.input_flags.d.exact_value;
    if (spec.input_flags.b.type == Value::EXACT) base.cpu.b = spec.input_flags.b.exact_value;
    if (spec.input_flags.v.type == Value::EXACT) base.cpu.v = spec.input_flags.v.exact_value;
    if (spec.input_flags.n.type == Value::EXACT) base.cpu.n = spec.input_flags.n.exact_value;
    
    // Set EXACT values for memory
    for (const auto& region : spec.input_memory) {
        for (size_t i = 0; i < region.bytes.size(); i++) {
            AddressT addr = static_cast<AddressT>(region.address + i);
            if (region.bytes[i].type == Value::EXACT) {
                base.memory_values[addr] = region.bytes[i].exact_value;
            }
        }
    }
    
    // Add base case
    test_cases.push_back(base);
    
    // Generate variations for ANY values
    // Strategy: Use boundary values
    std::vector<uint8_t> test_values = {0, 1, 0x7F, 0x80, 0xFF};
    
    // Function to create variations for a register
    auto createVariations = [&](Value value, std::function<void(TestCase&, uint8_t)> setter) {
        if (value.type == Value::ANY) {
            std::vector<TestCase> new_cases;
            for (const auto& test_case : test_cases) {
                for (uint8_t val : test_values) {
                    TestCase new_case = test_case;
                    setter(new_case, val);
                    new_cases.push_back(new_case);
                }
            }
            test_cases = std::move(new_cases);
        }
    };
    
    // Create variations for each ANY register
    createVariations(spec.input_cpu.a, [](TestCase& tc, uint8_t val) { tc.cpu.a = val; });
    createVariations(spec.input_cpu.x, [](TestCase& tc, uint8_t val) { tc.cpu.x = val; });
    createVariations(spec.input_cpu.y, [](TestCase& tc, uint8_t val) { tc.cpu.y = val; });
    createVariations(spec.input_cpu.sp, [](TestCase& tc, uint8_t val) { tc.cpu.sp = val; });
    
    // Create variations for ANY flags
    createVariations(spec.input_flags.c, [](TestCase& tc, uint8_t val) { tc.cpu.c = val != 0; });
    createVariations(spec.input_flags.z, [](TestCase& tc, uint8_t val) { tc.cpu.z = val != 0; });
    createVariations(spec.input_flags.i, [](TestCase& tc, uint8_t val) { tc.cpu.i = val != 0; });
    createVariations(spec.input_flags.d, [](TestCase& tc, uint8_t val) { tc.cpu.d = val != 0; });
    createVariations(spec.input_flags.b, [](TestCase& tc, uint8_t val) { tc.cpu.b = val != 0; });
    createVariations(spec.input_flags.v, [](TestCase& tc, uint8_t val) { tc.cpu.v = val != 0; });
    createVariations(spec.input_flags.n, [](TestCase& tc, uint8_t val) { tc.cpu.n = val != 0; });
    
    // Create variations for ANY memory values
    // This is a simplified approach to avoid combinatorial explosion
    for (const auto& region : spec.input_memory) {
        for (size_t i = 0; i < region.bytes.size(); i++) {
            AddressT addr = static_cast<AddressT>(region.address + i);
            
            // Skip if EXACT or already handled
            if (region.bytes[i].type == Value::EXACT || 
                std::any_of(test_cases.begin(), test_cases.end(),
                           [&](const TestCase& tc) { 
                               return tc.memory_values.find(addr) != tc.memory_values.end(); 
                           })) {
                continue;
            }
            
            // Check if the address affects output (simplified)
            bool affects_output = false;
            for (const auto& out_region : spec.output_memory) {
                if (out_region.containsAddress(addr) && 
                    (out_region.bytes[addr - out_region.address].type == Value::EXACT ||
                     out_region.bytes[addr - out_region.address].type == Value::SAME)) {
                    affects_output = true;
                    break;
                }
            }
            
            // Only create variations for important memory locations
            // to avoid combinatorial explosion
            if (affects_output) {
                // Use only a few representative values
                createVariations(Value::any(), [addr](TestCase& tc, uint8_t val) { 
                    tc.memory_values[addr] = val; 
                });
            }
        }
    }
    
    // Ensure critical edge cases are always included
    std::vector<uint8_t> critical_values = {0, 1, 0x7F, 0x80, 0xFF};
    
    // When sampling test cases, make sure these are included
    if (test_cases.size() > 100) {
        // First, extract test cases for critical values
        std::vector<TestCase> critical_cases;
        std::vector<TestCase> other_cases;
        
        for (const auto& test_case : test_cases) {
            bool is_critical = false;
            
            // Check if this test case uses any critical value
            for (const auto& [addr, value] : test_case.memory_values) {
                if (std::find(critical_values.begin(), critical_values.end(), value) 
                    != critical_values.end()) {
                    is_critical = true;
                    break;
                }
            }
            
            // Also consider CPU registers with critical values
            if (!is_critical) {
                if (std::find(critical_values.begin(), critical_values.end(), test_case.cpu.a) 
                    != critical_values.end()) {
                    is_critical = true;
                }
                else if (std::find(critical_values.begin(), critical_values.end(), test_case.cpu.x) 
                    != critical_values.end()) {
                    is_critical = true;
                }
                else if (std::find(critical_values.begin(), critical_values.end(), test_case.cpu.y) 
                    != critical_values.end()) {
                    is_critical = true;
                }
            }
            
            if (is_critical) {
                critical_cases.push_back(test_case);
            } else {
                other_cases.push_back(test_case);
            }
        }
        
        // Sample from non-critical cases
        std::vector<TestCase> sampled;
        size_t sample_size = std::min(static_cast<size_t>(100 - critical_cases.size()), 
                                      other_cases.size());
        
        std::sample(other_cases.begin(), other_cases.end(), 
                  std::back_inserter(sampled),
                  sample_size, 
                  std::mt19937{std::random_device{}()});
        
        // Combine critical and sampled cases
        test_cases = critical_cases;
        test_cases.insert(test_cases.end(), sampled.begin(), sampled.end());
    }
    
    return test_cases;
}

template<typename AddressT>
bool VerificationEngine<AddressT>::runTest(
    const std::vector<uint8_t>& sequence, 
    const TestCase& test) const {
    
    // Create CPU and memory for testing
    CPU6502 cpu;
    TrackedMemory<AddressT> memory;
    
    // Set up CPU state
    cpu.setState(test.cpu);
    
    // Set allowed regions for memory access
    memory.setInputRegions(spec.input_memory);
    memory.setOutputRegions(spec.output_memory);
    
    // Set up memory from test case
    for (const auto& [addr, value] : test.memory_values) {
        memory.initialize(addr, value);
    }
    
    // Copy sequence to execution address (use run_address from spec)
    for (size_t i = 0; i < sequence.size(); i++) {
        memory.initialize(static_cast<AddressT>(spec.run_address + i), sequence[i]);
    }
    
    try {
        // Execute sequence
        ExecutionResult result = cpu.execute(memory, spec.run_address);
        
        // Check for execution errors
        if (result.error != ExecutionResult::NONE) {
            return false;
        }
        
        // Get final state
        typename CPU6502::State final_state = cpu.getState();
        
        // Check CPU state
        if (!matchesCPUState(final_state, test, spec.output_cpu, spec.output_flags)) {
            return false;
        }
        
        // Check memory state
        if (!matchesMemoryState(memory, test, spec.output_memory)) {
            return false;
        }
        
        // Check for unauthorized memory modifications
        if (hasUnauthorizedModifications(memory, spec.output_memory)) {
            return false;
        }
        
        return true;
    }
    catch (const std::exception&) {
        // Any exception means the test failed
        return false;
    }
}

template<typename AddressT>
bool VerificationEngine<AddressT>::matchesCPUState(
    const typename CPU6502::State& actual,
    const TestCase& test,
    const typename OptimizationSpec<AddressT>::CPUState& expected_cpu,
    const typename OptimizationSpec<AddressT>::FlagState& expected_flags) const {
    
    // Check registers
    if (expected_cpu.a.type == Value::EXACT && actual.a != expected_cpu.a.exact_value) {
        return false;
    }
    if (expected_cpu.a.type == Value::SAME && actual.a != test.cpu.a) {
        return false;
    }
    
    if (expected_cpu.x.type == Value::EXACT && actual.x != expected_cpu.x.exact_value) {
        return false;
    }
    if (expected_cpu.x.type == Value::SAME && actual.x != test.cpu.x) {
        return false;
    }
    
    if (expected_cpu.y.type == Value::EXACT && actual.y != expected_cpu.y.exact_value) {
        return false;
    }
    if (expected_cpu.y.type == Value::SAME && actual.y != test.cpu.y) {
        return false;
    }
    
    if (expected_cpu.sp.type == Value::EXACT && actual.sp != expected_cpu.sp.exact_value) {
        return false;
    }
    if (expected_cpu.sp.type == Value::SAME && actual.sp != test.cpu.sp) {
        return false;
    }
    
    // Check flags
    if (expected_flags.c.type == Value::EXACT && actual.c != (expected_flags.c.exact_value != 0)) {
        return false;
    }
    if (expected_flags.c.type == Value::SAME && actual.c != test.cpu.c) {
        return false;
    }
    
    if (expected_flags.z.type == Value::EXACT && actual.z != (expected_flags.z.exact_value != 0)) {
        return false;
    }
    if (expected_flags.z.type == Value::SAME && actual.z != test.cpu.z) {
        return false;
    }
    
    if (expected_flags.i.type == Value::EXACT && actual.i != (expected_flags.i.exact_value != 0)) {
        return false;
    }
    if (expected_flags.i.type == Value::SAME && actual.i != test.cpu.i) {
        return false;
    }
    
    if (expected_flags.d.type == Value::EXACT && actual.d != (expected_flags.d.exact_value != 0)) {
        return false;
    }
    if (expected_flags.d.type == Value::SAME && actual.d != test.cpu.d) {
        return false;
    }
    
    if (expected_flags.b.type == Value::EXACT && actual.b != (expected_flags.b.exact_value != 0)) {
        return false;
    }
    if (expected_flags.b.type == Value::SAME && actual.b != test.cpu.b) {
        return false;
    }
    
    if (expected_flags.v.type == Value::EXACT && actual.v != (expected_flags.v.exact_value != 0)) {
        return false;
    }
    if (expected_flags.v.type == Value::SAME && actual.v != test.cpu.v) {
        return false;
    }
    
    if (expected_flags.n.type == Value::EXACT && actual.n != (expected_flags.n.exact_value != 0)) {
        return false;
    }
    if (expected_flags.n.type == Value::SAME && actual.n != test.cpu.n) {
        return false;
    }
    
    return true;
}

template<typename AddressT>
bool VerificationEngine<AddressT>::matchesMemoryState(
    const TrackedMemory<AddressT>& memory,
    const TestCase& test,
    const std::vector<typename OptimizationSpec<AddressT>::MemoryRegion>& expected) const {
    
    // Check all expected memory regions
    for (const auto& region : expected) {
        for (size_t i = 0; i < region.bytes.size(); i++) {
            AddressT addr = static_cast<AddressT>(region.address + i);
            
            if (region.bytes[i].type == Value::EXACT) {
                // This byte must match exactly
                try {
                    uint8_t actual = memory.read(addr);
                    uint8_t expected_value = region.bytes[i].exact_value;
                    
                    if (actual != expected_value) {
                        return false;
                    }
                }
                catch (const std::exception&) {
                    // Any exception means the test failed
                    return false;
                }
            }
            else if (region.bytes[i].type == Value::SAME) {
                // This byte must match the initial value
                try {
                    uint8_t actual = memory.read(addr);
                    
                    // Get the initial value from the test case
                    auto it = test.memory_values.find(addr);
                    if (it != test.memory_values.end()) {
                        uint8_t initial_value = it->second;
                        
                        if (actual != initial_value) {
                            return false;
                        }
                    }
                    // If not found in test case, it's an error
                    else {
                        return false;
                    }
                }
                catch (const std::exception&) {
                    // Any exception means the test failed
                    return false;
                }
            }
            // For ANY values, no need to check
        }
    }
    
    return true;
}

template<typename AddressT>
bool VerificationEngine<AddressT>::hasUnauthorizedModifications(
    const TrackedMemory<AddressT>& memory,
    const std::vector<typename OptimizationSpec<AddressT>::MemoryRegion>& allowed) const {
    
    auto modified = memory.getModifiedAddresses();
    
    for (AddressT addr : modified) {
        bool found = false;
        
        for (const auto& region : allowed) {
            if (region.containsAddress(addr)) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            return true; // Unauthorized modification
        }
    }
    
    return false;
}

template<typename AddressT>
bool VerificationEngine<AddressT>::runTestWithExplanation(
    const std::vector<uint8_t>& sequence, 
    const TestCase& test,
    std::string& explanation) const {
    
    // Create CPU and memory for testing
    CPU6502 cpu;
    TrackedMemory<AddressT> memory;
    
    // Set up CPU state
    cpu.setState(test.cpu);
    
    // Set allowed regions for memory access
    memory.setInputRegions(spec.input_memory);
    memory.setOutputRegions(spec.output_memory);
    
    // Set up memory from test case
    for (const auto& [addr, value] : test.memory_values) {
        memory.initialize(addr, value);
    }
    
    // Copy sequence to execution address
    for (size_t i = 0; i < sequence.size(); i++) {
        memory.initialize(static_cast<AddressT>(spec.run_address + i), sequence[i]);
    }
    
    try {
        // Execute sequence
        ExecutionResult result = cpu.execute(memory, spec.run_address);
        
        // Check for execution errors
        if (result.error != ExecutionResult::NONE) {
            explanation = "Execution error: " + result.error_message;
            return false;
        }
        
        // Get final state
        typename CPU6502::State final_state = cpu.getState();
        
        // Check CPU state
        if (!matchesCPUState(final_state, test, spec.output_cpu, spec.output_flags)) {
            std::stringstream oss;
            oss << "CPU state mismatch: ";
            
            // Check registers
            if (spec.output_cpu.a.type == Value::EXACT && final_state.a != spec.output_cpu.a.exact_value) {
                oss << "A=0x" << std::hex << static_cast<int>(final_state.a) << " (expected 0x" 
                    << static_cast<int>(spec.output_cpu.a.exact_value) << ") ";
            }
            else if (spec.output_cpu.a.type == Value::SAME && final_state.a != test.cpu.a) {
                oss << "A=0x" << std::hex << static_cast<int>(final_state.a) << " (expected SAME=0x" 
                    << static_cast<int>(test.cpu.a) << ") ";
            }
            
            if (spec.output_cpu.x.type == Value::EXACT && final_state.x != spec.output_cpu.x.exact_value) {
                oss << "X=0x" << std::hex << static_cast<int>(final_state.x) << " (expected 0x" 
                    << static_cast<int>(spec.output_cpu.x.exact_value) << ") ";
            }
            else if (spec.output_cpu.x.type == Value::SAME && final_state.x != test.cpu.x) {
                oss << "X=0x" << std::hex << static_cast<int>(final_state.x) << " (expected SAME=0x" 
                    << static_cast<int>(test.cpu.x) << ") ";
            }
            
            if (spec.output_cpu.y.type == Value::EXACT && final_state.y != spec.output_cpu.y.exact_value) {
                oss << "Y=0x" << std::hex << static_cast<int>(final_state.y) << " (expected 0x" 
                    << static_cast<int>(spec.output_cpu.y.exact_value) << ") ";
            }
            else if (spec.output_cpu.y.type == Value::SAME && final_state.y != test.cpu.y) {
                oss << "Y=0x" << std::hex << static_cast<int>(final_state.y) << " (expected SAME=0x" 
                    << static_cast<int>(test.cpu.y) << ") ";
            }
            
            if (spec.output_cpu.sp.type == Value::EXACT && final_state.sp != spec.output_cpu.sp.exact_value) {
                oss << "SP=0x" << std::hex << static_cast<int>(final_state.sp) << " (expected 0x" 
                    << static_cast<int>(spec.output_cpu.sp.exact_value) << ") ";
            }
            else if (spec.output_cpu.sp.type == Value::SAME && final_state.sp != test.cpu.sp) {
                oss << "SP=0x" << std::hex << static_cast<int>(final_state.sp) << " (expected SAME=0x" 
                    << static_cast<int>(test.cpu.sp) << ") ";
            }
            
            // Check flags
            if (spec.output_flags.c.type == Value::EXACT && 
                final_state.c != (spec.output_flags.c.exact_value != 0)) {
                oss << "C=" << (final_state.c ? "1" : "0") << " (expected " 
                    << (spec.output_flags.c.exact_value != 0 ? "1" : "0") << ") ";
            }
            else if (spec.output_flags.c.type == Value::SAME && final_state.c != test.cpu.c) {
                oss << "C=" << (final_state.c ? "1" : "0") << " (expected SAME=" 
                    << (test.cpu.c ? "1" : "0") << ") ";
            }
            
            // Similar checks for other flags...
            
            explanation = oss.str();
            return false;
        }
        
        // Check memory state
        if (!matchesMemoryState(memory, test, spec.output_memory)) {
            std::stringstream oss;
            oss << "Memory state mismatch: ";
            
            // Identify which memory regions had issues
            for (const auto& region : spec.output_memory) {
                for (size_t i = 0; i < region.bytes.size(); i++) {
                    AddressT addr = static_cast<AddressT>(region.address + i);
                    
                    if (region.bytes[i].type == Value::EXACT) {
                        try {
                            uint8_t actual = memory.read(addr);
                            uint8_t expected_value = region.bytes[i].exact_value;
                            
                            if (actual != expected_value) {
                                oss << "Addr 0x" << std::hex << static_cast<int>(addr) 
                                    << "=0x" << static_cast<int>(actual) 
                                    << " (expected 0x" << static_cast<int>(expected_value) << ") ";
                            }
                        }
                        catch (const std::exception&) {
                            oss << "Failed to read addr 0x" << std::hex << static_cast<int>(addr) << " ";
                        }
                    }
                    else if (region.bytes[i].type == Value::SAME) {
                        try {
                            uint8_t actual = memory.read(addr);
                            
                            // Get the initial value from the test case
                            auto it = test.memory_values.find(addr);
                            if (it != test.memory_values.end()) {
                                uint8_t initial_value = it->second;
                                
                                if (actual != initial_value) {
                                    oss << "Addr 0x" << std::hex << static_cast<int>(addr) 
                                        << "=0x" << static_cast<int>(actual) 
                                        << " (expected SAME=0x" << static_cast<int>(initial_value) << ") ";
                                }
                            }
                        }
                        catch (const std::exception&) {
                            oss << "Failed to read addr 0x" << std::hex << static_cast<int>(addr) << " ";
                        }
                    }
                }
            }
            
            explanation = oss.str();
            return false;
        }
        
        // Check for unauthorized memory modifications
        if (hasUnauthorizedModifications(memory, spec.output_memory)) {
            std::stringstream oss;
            oss << "Unauthorized memory modifications: ";
            
            auto modified = memory.getModifiedAddresses();
            for (AddressT addr : modified) {
                bool found = false;
                
                for (const auto& region : spec.output_memory) {
                    if (region.containsAddress(addr)) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    oss << "0x" << std::hex << static_cast<int>(addr) << " ";
                }
            }
            
            explanation = oss.str();
            return false;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        // Any exception means the test failed
        explanation = "Exception during execution: " + std::string(e.what());
        return false;
    }
}

// Explicit instantiation
template class VerificationEngine<uint16_t>;

} // namespace phaistos
