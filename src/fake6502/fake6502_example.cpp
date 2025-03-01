#include <iostream>
#include <array>
#include <memory>
#include <vector>
#include <cstring>
#include "fake6502.hpp"

// Simple 6502 system with 64K of RAM
class System6502 {
private:
    std::array<uint8_t, 65536> memory{};
    std::unique_ptr<Fake6502> cpu;
    
public:
    System6502() {
        // Initialize memory to zero
        memory.fill(0);
        
        // Setup CPU with memory read/write functions
        cpu = std::make_unique<Fake6502>(
            [this](uint16_t addr) -> uint8_t { 
                return this->readMemory(addr); 
            },
            [this](uint16_t addr, uint8_t val) { 
                this->writeMemory(addr, val); 
            }
        );
    }
    
    // Memory access functions
    uint8_t readMemory(uint16_t address) {
        return memory[address];
    }
    
    void writeMemory(uint16_t address, uint8_t value) {
        memory[address] = value;
    }
    
    // Load a program into memory
    void loadProgram(uint16_t address, const std::vector<uint8_t>& program) {
        if (address + program.size() <= memory.size()) {
            std::copy(program.begin(), program.end(), memory.begin() + address);
        }
    }
    
    // Reset the CPU
    void reset() {
        cpu->reset();
    }
    
    // Run the CPU for a specified number of cycles
    void run(uint32_t cycles) {
        cpu->exec(cycles);
    }
    
    // Execute a single instruction
    uint32_t step() {
        return cpu->step();
    }
    
    // Get CPU state for debugging
    void dumpState() {
        std::cout << "CPU State:\n";
        std::cout << "PC: $" << std::hex << cpu->getPC() << "\n";
        std::cout << "A: $" << std::hex << static_cast<int>(cpu->getA()) << "\n";
        std::cout << "X: $" << std::hex << static_cast<int>(cpu->getX()) << "\n";
        std::cout << "Y: $" << std::hex << static_cast<int>(cpu->getY()) << "\n";
        std::cout << "SP: $" << std::hex << static_cast<int>(cpu->getSP()) << "\n";
        std::cout << "Status: " << std::bitset<8>(cpu->getStatus()) << "\n";
        std::cout << "Cycles: " << std::dec << cpu->getClockTicks() << "\n";
        std::cout << "Instructions: " << std::dec << cpu->getInstructionCount() << "\n";
    }
    
    // Get direct access to the CPU
    Fake6502* getCPU() {
        return cpu.get();
    }
    
    // Get direct access to the memory
    std::array<uint8_t, 65536>& getMemory() {
        return memory;
    }
};

int main() {
    // Create a 6502 system
    System6502 system;
    
    // Simple program to add two numbers (3 + 5)
    // Equivalent 6502 assembly:
    //   LDA #$03  ; Load 3 into accumulator
    //   ADC #$05  ; Add 5 to accumulator
    //   STA $0200 ; Store result at address $0200
    //   BRK       ; Break
    std::vector<uint8_t> program = {
        0xA9, 0x03,  // LDA #$03
        0x69, 0x05,  // ADC #$05
        0x8D, 0x00, 0x02,  // STA $0200
        0x00         // BRK
    };
    
    // Set reset vector to point to our program at $1000
    system.writeMemory(0xFFFC, 0x00);
    system.writeMemory(0xFFFD, 0x10);
    
    // Load program at address $1000
    system.loadProgram(0x1000, program);
    
    // Reset the CPU - this will load PC from the reset vector
    system.reset();
    
    std::cout << "Initial state:" << std::endl;
    system.dumpState();
    
    // Execute each instruction one by one
    while (system.getCPU()->getPC() < 0x1000 + program.size()) {
        uint32_t cycles = system.step();
        std::cout << "\nExecuted instruction, used " << cycles << " cycles." << std::endl;
        system.dumpState();
    }
    
    // Check the result
    std::cout << "\nResult at $0200: " << static_cast<int>(system.readMemory(0x0200)) << std::endl;
    
    return 0;
}
