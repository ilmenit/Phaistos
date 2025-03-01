/**
 * @file optimizer.hpp
 * @brief Core optimization engine
 */
#pragma once

#include "common.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "optimization_spec.hpp"
#include "sequence_generator.hpp"
#include "transformation_cache.hpp"
#include "verification_engine.hpp"

namespace phaistos {

/**
 * @class Optimizer
 * @brief Core optimization engine for 6502 code
 */
template<typename AddressT = uint16_t>
class Optimizer {
public:
    /**
     * @brief Constructor
     * @param spec Optimization specification
     */
    Optimizer(const OptimizationSpec<AddressT>& spec);
    
    /**
     * @brief Start the optimization process
     * @param timeout_seconds Maximum time to spend optimizing (in seconds)
     * @return The optimized instruction sequence
     */
    std::vector<uint8_t> optimize(int timeout_seconds = 300);
    
    /**
     * @struct ProgressListener
     * @brief Listener for optimization progress
     */
    struct ProgressListener {
        virtual ~ProgressListener() = default;
        
        /**
         * @brief Called when a new best solution is found
         * @param solution The current best solution
         * @param metric The metric value (size or cycles)
         * @param sequences_tested Number of sequences tested so far
         */
        virtual void onNewBestSolution(const std::vector<uint8_t>& solution, 
                                     size_t metric,
                                     size_t sequences_tested) = 0;
        
        /**
         * @brief Called periodically to report progress
         * @param sequences_tested Number of sequences tested so far
         * @param valid_sequences_found Number of valid sequences found
         * @param cache_size Size of the transformation cache
         */
        virtual void onProgress(size_t sequences_tested,
                              size_t valid_sequences_found,
                              size_t cache_size) = 0;
    };
    
    /**
     * @brief Set a progress listener
     * @param listener Listener to set
     */
    void setProgressListener(ProgressListener* listener);
    
private:
    /**
     * @brief Reference to the optimization specification
     */
    const OptimizationSpec<AddressT>& spec;
    
    /**
     * @brief Progress listener (may be null)
     */
    ProgressListener* progress_listener;
    
    /**
     * @brief Verification engine
     */
    VerificationEngine<AddressT> verifier;
    
    /**
     * @brief Sequence generator
     */
    SequenceGenerator generator;
    
    /**
     * @brief Transformation cache
     */
    TransformationCache<AddressT> cache;
    
    /**
     * @brief Optimize a sequence using the cache
     * @param sequence Original sequence
     * @return Optimized sequence
     */
    std::vector<uint8_t> optimizeWithCache(const std::vector<uint8_t>& sequence);
    
    /**
     * @brief Extract a transformation from a sequence
     * @param sequence Instruction sequence
     * @return Transformation key
     */
    typename TransformationCache<AddressT>::TransformationKey 
    extractTransformation(const std::vector<uint8_t>& sequence);
    
    /**
     * @brief Extract a transformation for a subsequence
     * @param full_sequence Full instruction sequence
     * @param start Start index
     * @param end End index
     * @return Transformation key
     */
    typename TransformationCache<AddressT>::TransformationKey 
    extractSubsequenceTransformation(const std::vector<uint8_t>& full_sequence,
                                   size_t start,
                                   size_t end);
    
    /**
     * @brief Find instruction boundaries in a sequence
     * @param sequence Instruction sequence
     * @return Vector of boundary indices
     */
    std::vector<size_t> findInstructionBoundaries(const std::vector<uint8_t>& sequence);
    
    /**
     * @brief Get the size of an instruction
     * @param opcode Instruction opcode
     * @return Size in bytes
     */
    size_t getInstructionSize(uint8_t opcode);
};

/**
 * @brief Simple console progress listener implementation
 */
class ConsoleProgressListener : public Optimizer<uint16_t>::ProgressListener {
public:
    /**
     * @brief Called when a new best solution is found
     * @param solution The current best solution
     * @param metric The metric value (size or cycles)
     * @param sequences_tested Number of sequences tested so far
     */
    void onNewBestSolution(const std::vector<uint8_t>& solution, 
                         size_t metric,
                         size_t sequences_tested) override;
    
    /**
     * @brief Called periodically to report progress
     * @param sequences_tested Number of sequences tested so far
     * @param valid_sequences_found Number of valid sequences found
     * @param cache_size Size of the transformation cache
     */
    void onProgress(size_t sequences_tested,
                  size_t valid_sequences_found,
                  size_t cache_size) override;
};

} // namespace phaistos
