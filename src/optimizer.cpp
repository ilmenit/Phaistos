/**
 * @file optimizer.cpp
 * @brief Implementation of the core optimization engine
 */
#include "optimizer.hpp"

namespace phaistos {

template<typename AddressT>
Optimizer<AddressT>::Optimizer(const OptimizationSpec<AddressT>& spec)
    : spec(spec), progress_listener(nullptr), verifier(spec) {
}

template<typename AddressT>
std::vector<uint8_t> Optimizer<AddressT>::optimize(int timeout_seconds) {
    // Configure sequence generator
    generator.setMaxLength(32);  // Start with a reasonable limit
    
    // Use CPU architecture to determine valid opcodes
    std::vector<uint8_t> valid_opcodes = getAllValidOpcodes();
    generator.setValidOpcodes(valid_opcodes);
    
    // Initialize cache
    cache.clear();
    
    // Set timeout
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(timeout_seconds);
    
    // Track best solution
    std::vector<uint8_t> best_solution;
    size_t best_metric = std::numeric_limits<size_t>::max();
    
    // Statistics
    size_t sequences_tested = 0;
    size_t valid_sequences_found = 0;
    bool done = false;
    
    // Optimization loop
    std::vector<uint8_t> candidate;
    while (!done && generator.next(candidate)) {
        // Check timeout
        auto now = std::chrono::steady_clock::now();
        if (now > end_time) {
            if (progress_listener) {
                progress_listener->onProgress(sequences_tested, valid_sequences_found, cache.size());
            }
            break;
        }
        
        // Try to optimize using cache first
        std::vector<uint8_t> optimized = optimizeWithCache(candidate);
        
        // Verify this sequence
        sequences_tested++;
        
        if (verifier.verify(optimized)) {
            valid_sequences_found++;
            
            // Calculate metric based on goal
            size_t metric;
            if (spec.goal == OptimizationSpec<AddressT>::SIZE) {
                metric = verifier.getSize(optimized);
            } else {
                metric = verifier.getCycles(optimized);
            }
            
            // Update if better
            if (metric < best_metric) {
                best_solution = optimized;
                best_metric = metric;
                
                if (progress_listener) {
                    progress_listener->onNewBestSolution(best_solution, metric, sequences_tested);
                }
                
                // If optimizing for size, we can stop once we find any valid solution
                // since we're generating sequences in order of increasing length
                if (spec.goal == OptimizationSpec<AddressT>::SIZE) {
                    done = true;
                    break;
                }
                
                // For speed optimization, we should continue searching but can limit the search space
                if (spec.goal == OptimizationSpec<AddressT>::SPEED) {
                    // Limit to sequences no more than 4 bytes larger than the best solution
                    generator.setMaxLength(best_solution.size() + 4);
                    
                    // If we find a sequence significantly larger than the best solution,
                    // we can stop the search
                    if (optimized.size() > best_solution.size() + 4) {
                        done = true;
                        break;
                    }
                }
            }
            
            // Add to cache for future use
            auto key = extractTransformation(optimized);
            cache.add(key, optimized, verifier.getCycles(optimized));
        }
        
        // Report progress periodically
        if (progress_listener && sequences_tested % 1000 == 0) {
            progress_listener->onProgress(sequences_tested, valid_sequences_found, cache.size());
        }
    }
    
    // Final progress report
    if (progress_listener) {
        progress_listener->onProgress(sequences_tested, valid_sequences_found, cache.size());
    }
    
    return best_solution;
}

template<typename AddressT>
void Optimizer<AddressT>::setProgressListener(ProgressListener* listener) {
    progress_listener = listener;
}

template<typename AddressT>
std::vector<uint8_t> Optimizer<AddressT>::optimizeWithCache(const std::vector<uint8_t>& sequence) {
    // Try to optimize the full sequence
    auto key = extractTransformation(sequence);
    auto cached = cache.findOptimal(key, spec.goal == OptimizationSpec<AddressT>::SIZE);
    
    if (cached) {
        return *cached;
    }
    
    // Divide and conquer approach: Try to optimize subsequences
    std::vector<size_t> boundaries = findInstructionBoundaries(sequence);
    
    // Skip if too short
    if (boundaries.size() <= 2) {
        return sequence;
    }
    
    // Optimize each subsequence
    bool optimized = false;
    std::vector<uint8_t> result = sequence;
    
    for (size_t i = 0; i < boundaries.size() - 1; i++) {
        for (size_t j = i + 1; j < boundaries.size(); j++) {
            size_t start = boundaries[i];
            size_t end = boundaries[j];
            
            // Skip tiny subsequences
            if (end - start <= 2) {
                continue;
            }
            
            // Extract subsequence
            std::vector<uint8_t> subsequence(sequence.begin() + static_cast<std::ptrdiff_t>(start), 
                                            sequence.begin() + static_cast<std::ptrdiff_t>(end));
            
            // Try to find an optimal replacement
            auto subkey = extractSubsequenceTransformation(sequence, start, end);
            auto replacement = cache.findOptimal(subkey, spec.goal == OptimizationSpec<AddressT>::SIZE);
            
            if (replacement && replacement->size() < (end - start)) {
                // Replace subsequence with optimized version
                std::vector<uint8_t> new_result;
                new_result.reserve(result.size() - (end - start) + replacement->size());
                
                new_result.insert(new_result.end(), result.begin(), 
                                 result.begin() + static_cast<std::ptrdiff_t>(start));
                new_result.insert(new_result.end(), replacement->begin(), replacement->end());
                new_result.insert(new_result.end(), 
                                 result.begin() + static_cast<std::ptrdiff_t>(end), result.end());
                
                result = std::move(new_result);
                optimized = true;
                
                // Recalculate boundaries after replacement
                boundaries = findInstructionBoundaries(result);
                i = 0;  // Start over
                break;
            }
        }
    }
    
    if (optimized) {
        // Try again recursively
        return optimizeWithCache(result);
    }
    
    return result;
}

template<typename AddressT>
typename TransformationCache<AddressT>::TransformationKey 
Optimizer<AddressT>::extractTransformation(const std::vector<uint8_t>& sequence) {
    // Create CPU and memory for execution
    CPU6502 cpu;
    TrackedMemory<AddressT> memory;
    
    // Set initial state
    typename CPU6502::State initial_state;
    
    // Set registers from spec
    if (spec.input_cpu.a.type == Value::EXACT) initial_state.a = spec.input_cpu.a.exact_value;
    if (spec.input_cpu.x.type == Value::EXACT) initial_state.x = spec.input_cpu.x.exact_value;
    if (spec.input_cpu.y.type == Value::EXACT) initial_state.y = spec.input_cpu.y.exact_value;
    if (spec.input_cpu.sp.type == Value::EXACT) initial_state.sp = spec.input_cpu.sp.exact_value;
    
    // Set flags from spec
    if (spec.input_flags.c.type == Value::EXACT) initial_state.c = spec.input_flags.c.exact_value;
    if (spec.input_flags.z.type == Value::EXACT) initial_state.z = spec.input_flags.z.exact_value;
    if (spec.input_flags.i.type == Value::EXACT) initial_state.i = spec.input_flags.i.exact_value;
    if (spec.input_flags.d.type == Value::EXACT) initial_state.d = spec.input_flags.d.exact_value;
    if (spec.input_flags.b.type == Value::EXACT) initial_state.b = spec.input_flags.b.exact_value;
    if (spec.input_flags.v.type == Value::EXACT) initial_state.v = spec.input_flags.v.exact_value;
    if (spec.input_flags.n.type == Value::EXACT) initial_state.n = spec.input_flags.n.exact_value;
    
    cpu.setState(initial_state);
    
    // Set up memory with exact values only
    for (const auto& region : spec.input_memory) {
        for (size_t i = 0; i < region.bytes.size(); i++) {
            if (region.bytes[i].type == Value::EXACT) {
                memory.initialize(region.address + static_cast<AddressT>(i), region.bytes[i].exact_value);
            }
        }
    }
    
    // Run the sequence
    for (size_t i = 0; i < sequence.size(); i++) {
        memory.initialize(spec.run_address + static_cast<AddressT>(i), sequence[i]);
    }
    
    try {
        ExecutionResult result = cpu.execute(memory, spec.run_address);
        
        // Check for execution errors
        if (result.error != ExecutionResult::NONE) {
            throw std::runtime_error("Execution error: " + result.error_message);
        }
        
        // Extract transformation
        typename TransformationCache<AddressT>::TransformationKey key;
        
        // Input state
        if (spec.input_cpu.a.type == Value::EXACT) key.input.registers["A"] = spec.input_cpu.a.exact_value;
        if (spec.input_cpu.x.type == Value::EXACT) key.input.registers["X"] = spec.input_cpu.x.exact_value;
        if (spec.input_cpu.y.type == Value::EXACT) key.input.registers["Y"] = spec.input_cpu.y.exact_value;
        if (spec.input_cpu.sp.type == Value::EXACT) key.input.registers["SP"] = spec.input_cpu.sp.exact_value;
        
        for (const auto& region : spec.input_memory) {
            for (size_t i = 0; i < region.bytes.size(); i++) {
                if (region.bytes[i].type == Value::EXACT) {
                    key.input.memory[region.address + static_cast<AddressT>(i)] = region.bytes[i].exact_value;
                }
            }
        }
        
        // Output state
        typename CPU6502::State final_state = cpu.getState();
        key.output.registers["A"] = final_state.a;
        key.output.registers["X"] = final_state.x;
        key.output.registers["Y"] = final_state.y;
        key.output.registers["SP"] = final_state.sp;
        
        // Only include memory that was modified
        for (AddressT addr : memory.getModifiedAddresses()) {
            key.output.memory[addr] = memory.read(addr);
        }
        
        return key;
    }
    catch (const std::exception&) {
        // If execution fails, return an empty key
        return typename TransformationCache<AddressT>::TransformationKey();
    }
}

template<typename AddressT>
typename TransformationCache<AddressT>::TransformationKey 
Optimizer<AddressT>::extractSubsequenceTransformation(
    const std::vector<uint8_t>& full_sequence, 
    size_t start, 
    size_t end) {
    
    // Extract the subsequence
    std::vector<uint8_t> subsequence(full_sequence.begin() + static_cast<std::ptrdiff_t>(start), 
                                    full_sequence.begin() + static_cast<std::ptrdiff_t>(end));
    
    // For simplicity, we'll just execute the subsequence in isolation
    return extractTransformation(subsequence);
}

template<typename AddressT>
std::vector<size_t> Optimizer<AddressT>::findInstructionBoundaries(const std::vector<uint8_t>& sequence) {
    std::vector<size_t> boundaries;
    boundaries.push_back(0);  // Start
    
    size_t pos = 0;
    while (pos < sequence.size()) {
        uint8_t opcode = sequence[pos];
        size_t size = getInstructionSize(opcode);
        
        pos += size;
        if (pos <= sequence.size()) {
            boundaries.push_back(pos);
        }
    }
    
    return boundaries;
}

template<typename AddressT>
size_t Optimizer<AddressT>::getInstructionSize(uint8_t opcode) {
    // Simplified instruction size lookup
    static const std::unordered_map<uint8_t, uint8_t> sizes = {
        {0x00, 1}, {0x01, 2}, {0x05, 2}, {0x06, 2}, {0x08, 1}, {0x09, 2}, {0x0A, 1},
        {0x0D, 3}, {0x0E, 3}, {0x10, 2}, {0x11, 2}, {0x15, 2}, {0x16, 2}, {0x18, 1},
        {0x19, 3}, {0x1D, 3}, {0x1E, 3}, {0x20, 3}, {0x21, 2}, {0x24, 2}, {0x25, 2},
        {0x26, 2}, {0x28, 1}, {0x29, 2}, {0x2A, 1}, {0x2C, 3}, {0x2D, 3}, {0x2E, 3},
        {0x30, 2}, {0x31, 2}, {0x35, 2}, {0x36, 2}, {0x38, 1}, {0x39, 3}, {0x3D, 3},
        {0x3E, 3}, {0x40, 1}, {0x41, 2}, {0x45, 2}, {0x46, 2}, {0x48, 1}, {0x49, 2},
        {0x4A, 1}, {0x4C, 3}, {0x4D, 3}, {0x4E, 3}, {0x50, 2}, {0x51, 2}, {0x55, 2},
        {0x56, 2}, {0x58, 1}, {0x59, 3}, {0x5D, 3}, {0x5E, 3}, {0x60, 1}, {0x61, 2},
        {0x65, 2}, {0x66, 2}, {0x68, 1}, {0x69, 2}, {0x6A, 1}, {0x6C, 3}, {0x6D, 3},
        {0x6E, 3}, {0x70, 2}, {0x71, 2}, {0x75, 2}, {0x76, 2}, {0x78, 1}, {0x79, 3},
        {0x7D, 3}, {0x7E, 3}, {0x81, 2}, {0x84, 2}, {0x85, 2}, {0x86, 2}, {0x88, 1},
        {0x8A, 1}, {0x8C, 3}, {0x8D, 3}, {0x8E, 3}, {0x90, 2}, {0x91, 2}, {0x94, 2},
        {0x95, 2}, {0x96, 2}, {0x98, 1}, {0x99, 3}, {0x9A, 1}, {0x9D, 3}, {0xA0, 2},
        {0xA1, 2}, {0xA2, 2}, {0xA4, 2}, {0xA5, 2}, {0xA6, 2}, {0xA8, 1}, {0xA9, 2},
        {0xAA, 1}, {0xAC, 3}, {0xAD, 3}, {0xAE, 3}, {0xB0, 2}, {0xB1, 2}, {0xB4, 2},
        {0xB5, 2}, {0xB6, 2}, {0xB8, 1}, {0xB9, 3}, {0xBA, 1}, {0xBC, 3}, {0xBD, 3},
        {0xBE, 3}, {0xC0, 2}, {0xC1, 2}, {0xC4, 2}, {0xC5, 2}, {0xC6, 2}, {0xC8, 1},
        {0xC9, 2}, {0xCA, 1}, {0xCC, 3}, {0xCD, 3}, {0xCE, 3}, {0xD0, 2}, {0xD1, 2},
        {0xD5, 2}, {0xD6, 2}, {0xD8, 1}, {0xD9, 3}, {0xDD, 3}, {0xDE, 3}, {0xE0, 2},
        {0xE1, 2}, {0xE4, 2}, {0xE5, 2}, {0xE6, 2}, {0xE8, 1}, {0xE9, 2}, {0xEA, 1},
        {0xEC, 3}, {0xED, 3}, {0xEE, 3}, {0xF0, 2}, {0xF1, 2}, {0xF5, 2}, {0xF6, 2},
        {0xF8, 1}, {0xF9, 3}, {0xFD, 3}, {0xFE, 3}
    };
    
    auto it = sizes.find(opcode);
    if (it != sizes.end()) {
        return it->second;
    }
    
    // Default size (NOP or unknown)
    return 1;
}

// Console progress listener implementation
void ConsoleProgressListener::onNewBestSolution(
    const std::vector<uint8_t>& solution, 
    size_t metric, 
    size_t sequences_tested) {
    
    std::cout << "New best solution found after testing " << sequences_tested << " sequences:" << std::endl;
    std::cout << "  Size: " << solution.size() << " bytes" << std::endl;
    std::cout << "  Metric: " << metric << std::endl;
    
    // Print bytes as hex
    std::cout << "  Bytes: ";
    for (uint8_t b : solution) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b) << " ";
    }
    std::cout << std::dec << std::endl;
}

void ConsoleProgressListener::onProgress(
    size_t sequences_tested, 
    size_t valid_sequences_found, 
    size_t cache_size) {
    
    std::cout << "Progress update:" << std::endl;
    std::cout << "  Sequences tested: " << sequences_tested << std::endl;
    std::cout << "  Valid sequences found: " << valid_sequences_found << std::endl;
    std::cout << "  Cache size: " << cache_size << std::endl;
}

// Explicit instantiation
template class Optimizer<uint16_t>;

} // namespace phaistos
