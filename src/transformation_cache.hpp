/**
 * @file transformation_cache.hpp
 * @brief Cache for code transformations
 */
#pragma once

#include "common.hpp"

namespace phaistos {

    /**
     * @class TransformationCache
     * @brief Caches the optimal sequences for specific transformations
     */
    template<typename AddressT = uint16_t>
    class TransformationCache {
    public:
        /**
         * @struct StateDescription
         * @brief Description of CPU and memory state
         */
        struct StateDescription {
            std::map<std::string, uint8_t> registers;  // Register name to value
            std::map<AddressT, uint8_t> memory;        // Address to value

            /**
             * @brief Equality comparison
             * @param other Other state to compare with
             * @return True if states are equal
             */
            bool operator==(const StateDescription& other) const {
                return registers == other.registers && memory == other.memory;
            }
        };

        /**
         * @struct TransformationKey
         * @brief Key representing a state transformation
         */
        struct TransformationKey {
            StateDescription input;
            StateDescription output;

            /**
             * @brief Equality comparison
             * @param other Other key to compare with
             * @return True if keys are equal
             */
            bool operator==(const TransformationKey& other) const {
                return input == other.input && output == other.output;
            }
        };

        /**
         * @struct KeyHasher
         * @brief Hash function for TransformationKey
         */
        struct KeyHasher {
            /**
             * @brief Calculate hash for a transformation key
             * @param key Key to hash
             * @return Hash value
             */
            size_t operator()(const TransformationKey& key) const {
                size_t h = 0;

                // Hash input registers
                for (const auto& [name, value] : key.input.registers) {
                    h = h * 31 + std::hash<std::string>{}(name);
                    h = h * 31 + value;
                }

                // Hash input memory
                for (const auto& [addr, value] : key.input.memory) {
                    h = h * 31 + std::hash<AddressT>{}(addr);
                    h = h * 31 + value;
                }

                // Hash output registers
                for (const auto& [name, value] : key.output.registers) {
                    h = h * 31 + std::hash<std::string>{}(name);
                    h = h * 31 + value;
                }

                // Hash output memory
                for (const auto& [addr, value] : key.output.memory) {
                    h = h * 31 + std::hash<AddressT>{}(addr);
                    h = h * 31 + value;
                }

                return h;
            }
        };

        /**
         * @brief Add a sequence to the cache
         * @param key Transformation key
         * @param sequence Instruction sequence
         * @param cycles Cycle count
         */
        void add(const TransformationKey& key,
            const std::vector<uint8_t>& sequence,
            size_t cycles);

        /**
         * @brief Find the optimal sequence for a transformation
         * @param key Transformation key
         * @param optimize_for_size Whether to optimize for size
         * @return Optional containing the optimal sequence if found
         */
        std::optional<std::vector<uint8_t>> findOptimal(
            const TransformationKey& key, bool optimize_for_size);

        /**
         * @brief Clear the cache
         */
        void clear();

        /**
         * @brief Get the current size of the cache
         * @return Number of entries in the cache
         */
        size_t size() const;

    private:
        /**
         * @struct CacheEntry
         * @brief Entry in the cache with optimal sequences
         */
        struct CacheEntry {
            std::vector<uint8_t> size_optimal_sequence;
            size_t size_optimal_cycles;

            std::vector<uint8_t> cycle_optimal_sequence;
            size_t cycle_optimal_cycles;  // Added this field to store cycle count correctly
        };

        /** The cache storage */
        std::unordered_map<TransformationKey, CacheEntry, KeyHasher> cache;
    };

} // namespace phaistos