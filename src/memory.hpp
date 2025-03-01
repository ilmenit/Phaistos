/**
 * @file memory.hpp
 * @brief Memory interface and implementations
 */
#pragma once

#include "common.hpp"
#include "optimization_spec.hpp"

namespace phaistos {

/**
 * @class Memory
 * @brief Generic memory interface with configurable address type
 */
template<typename AddressT = uint16_t>
class Memory {
public:
    virtual ~Memory() = default;
    
    /**
     * @brief Read a byte from memory
     * @param address The address to read from
     * @return The byte value at the address
     */
    virtual uint8_t read(AddressT address) const = 0;
    
    /**
     * @brief Write a byte to memory
     * @param address The address to write to
     * @param value The value to write
     */
    virtual void write(AddressT address, uint8_t value) = 0;
    
    /**
     * @brief Read a 16-bit word from memory (little-endian)
     * @param address The address to read from
     * @return The 16-bit value at the address
     */
    virtual uint16_t read16(AddressT address) const {
        return static_cast<uint16_t>(read(address)) | 
               (static_cast<uint16_t>(read(static_cast<AddressT>(address + 1))) << 8);
    }
    
    /**
     * @brief Write a 16-bit word to memory (little-endian)
     * @param address The address to write to
     * @param value The value to write
     */
    virtual void write16(AddressT address, uint16_t value) {
        write(address, static_cast<uint8_t>(value & 0xFF));
        write(static_cast<AddressT>(address + 1), static_cast<uint8_t>(value >> 8));
    }
};

/**
 * @class TrackedMemory
 * @brief Memory implementation that tracks reads and writes
 */
template<typename AddressT = uint16_t>
class TrackedMemory : public Memory<AddressT> {
public:
    /**
     * @brief Read a byte from memory
     * @param address The address to read from
     * @return The byte value at the address
     * @throws std::runtime_error If address is not in allowed input regions
     */
    uint8_t read(AddressT address) const override {
        // Record the read access (need to cast away const-ness for internal tracking)
        const_cast<TrackedMemory<AddressT>*>(this)->read_addresses.insert(address);
        
        // Verify this address is defined in input memory regions
        if (!isReadAllowed(address)) {
            throw std::runtime_error("Memory read violation: Address " + 
                                    std::to_string(address) + 
                                    " not defined in input memory regions");
        }
        
        // Access from sparse memory map
        auto it = memory.find(address);
        if (it != memory.end()) {
            return it->second;
        }
        return 0; // Default for uninitialized memory
    }
    
    /**
     * @brief Write a byte to memory
     * @param address The address to write to
     * @param value The value to write
     * @throws std::runtime_error If address is not in allowed output regions
     */
    void write(AddressT address, uint8_t value) override {
        // Record the write access
        modified_addresses.insert(address);
        
        // Check for self-modifying code (address was previously read)
        if (read_addresses.count(address) > 0) {
            // This address was read before, so this is potentially self-modifying code
            if (!isReadWriteAllowed(address)) {
                throw std::runtime_error("Memory write violation: Self-modifying code at address " + 
                                        std::to_string(address) + 
                                        " but address not defined in both input and output memory regions");
            }
        }
        
        // Verify this address is defined in output memory regions
        if (!isWriteAllowed(address)) {
            throw std::runtime_error("Memory write violation: Address " + 
                                    std::to_string(address) + 
                                    " not defined in output memory regions");
        }
        
        // Write to sparse memory map
        memory[address] = value;
    }
    
    /**
     * @brief Get the set of addresses that have been modified
     * @return Set of modified addresses
     */
    std::unordered_set<AddressT> getModifiedAddresses() const {
        return modified_addresses;
    }
    
    /**
     * @brief Get the set of addresses that have been read
     * @return Set of read addresses
     */
    std::unordered_set<AddressT> getReadAddresses() const {
        return read_addresses;
    }
    
    /**
     * @brief Set the allowed input memory regions
     * @param regions The allowed memory regions for reading
     */
    void setInputRegions(const std::vector<typename OptimizationSpec<AddressT>::MemoryRegion>& regions) {
        input_regions = regions;
    }
    
    /**
     * @brief Set the allowed output memory regions
     * @param regions The allowed memory regions for writing
     */
    void setOutputRegions(const std::vector<typename OptimizationSpec<AddressT>::MemoryRegion>& regions) {
        output_regions = regions;
    }
    
    /**
     * @brief Initialize memory with a value at an address
     * @param address The address to initialize
     * @param value The value to set
     */
    void initialize(AddressT address, uint8_t value) {
        memory[address] = value;
    }
    
private:
    // Sparse memory model for all addresses
    mutable std::unordered_map<AddressT, uint8_t> memory;
    
    // Access tracking
    mutable std::unordered_set<AddressT> modified_addresses;
    mutable std::unordered_set<AddressT> read_addresses;
    
    // Allowed memory regions
    std::vector<typename OptimizationSpec<AddressT>::MemoryRegion> input_regions;
    std::vector<typename OptimizationSpec<AddressT>::MemoryRegion> output_regions;
    
    /**
     * @brief Check if reading from an address is allowed
     * @param address The address to check
     * @return True if the address can be read
     */
    bool isReadAllowed(AddressT address) const {
        for (const auto& region : input_regions) {
            if (region.containsAddress(address)) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Check if writing to an address is allowed
     * @param address The address to check
     * @return True if the address can be written
     */
    bool isWriteAllowed(AddressT address) const {
        for (const auto& region : output_regions) {
            if (region.containsAddress(address)) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Check if an address can be both read and written (for self-modifying code)
     * @param address The address to check
     * @return True if the address can be both read and written
     */
    bool isReadWriteAllowed(AddressT address) const {
        return isReadAllowed(address) && isWriteAllowed(address);
    }
};

} // namespace phaistos
