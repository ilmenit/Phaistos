/**
 * @file transformation_cache.cpp
 * @brief Implementation of the transformation cache
 */
#include "transformation_cache.hpp"

namespace phaistos {

template<typename AddressT>
void TransformationCache<AddressT>::add(
    const TransformationKey& key, 
    const std::vector<uint8_t>& sequence,
    size_t cycles) {
    
    auto it = cache.find(key);
    
    if (it == cache.end()) {
        // First sequence for this transformation
        CacheEntry entry;
        entry.size_optimal_sequence = sequence;
        entry.size_optimal_cycles = cycles;
        entry.cycle_optimal_sequence = sequence;
        entry.cycle_optimal_size = sequence.size();
        
        cache[key] = entry;
    } else {
        // Check if this sequence is better for size
        if (sequence.size() < it->second.size_optimal_sequence.size()) {
            it->second.size_optimal_sequence = sequence;
            it->second.size_optimal_cycles = cycles;
        }
        
        // Check if this sequence is better for cycles
        if (cycles < it->second.cycle_optimal_sequence.size()) {
            it->second.cycle_optimal_sequence = sequence;
            it->second.cycle_optimal_size = sequence.size();
        }
    }
}

template<typename AddressT>
std::optional<std::vector<uint8_t>> TransformationCache<AddressT>::findOptimal(
    const TransformationKey& key, bool optimize_for_size) {
    
    auto it = cache.find(key);
    if (it == cache.end()) {
        return std::nullopt;
    }
    
    if (optimize_for_size) {
        return it->second.size_optimal_sequence;
    } else {
        return it->second.cycle_optimal_sequence;
    }
}

template<typename AddressT>
void TransformationCache<AddressT>::clear() {
    cache.clear();
}

template<typename AddressT>
size_t TransformationCache<AddressT>::size() const {
    return cache.size();
}

// Explicit instantiation
template class TransformationCache<uint16_t>;

} // namespace phaistos
