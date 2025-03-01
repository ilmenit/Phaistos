/**
 * Fake6502 CPU emulator core v2.0
 * 
 * Original C implementation by Mike Chambers (miker00lz@gmail.com)
 * Modified by David MHS Webster (github.com/gek169)
 * 
 * This is a modernized C++ version of the Fake6502 emulator with
 * thread-safety improvements, proper encapsulation, and better C++ practices
 * while maintaining compatibility and performance.
 */

#ifndef FAKE6502_HPP
#define FAKE6502_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

class Fake6502 {
public:
    /**
     * Create a new 6502 CPU emulator
     * 
     * @param memoryRead Function to read a byte from memory
     * @param memoryWrite Function to write a byte to memory
     */
    Fake6502(
        std::function<uint8_t(uint16_t)> memoryRead,
        std::function<void(uint16_t, uint8_t)> memoryWrite
    );

    /**
     * Default destructor
     */
    ~Fake6502() = default;

    /**
     * Deleted copy constructor and assignment operator to prevent copying
     */
    Fake6502(const Fake6502&) = delete;
    Fake6502& operator=(const Fake6502&) = delete;

    /**
     * Reset the CPU to its initial state
     */
    void reset();

    /**
     * Execute CPU instructions for the specified number of clock cycles
     * 
     * @param tickCount Number of clock cycles to execute
     * @return Actual number of clock cycles executed
     */
    uint32_t exec(uint32_t tickCount);

    /**
     * Execute a single instruction
     * 
     * @return Number of clock cycles used
     */
    uint32_t step();

    /**
     * Trigger a hardware IRQ in the CPU
     */
    void irq();

    /**
     * Trigger an NMI in the CPU
     */
    void nmi();

    /**
     * Set a hook function that will be called after every instruction
     * 
     * @param hook Function to call, or std::nullopt to disable
     */
    void setHook(std::optional<std::function<void()>> hook);

    /**
     * Get the current program counter
     * 
     * @return The program counter value
     */
    uint16_t getPC() const { return pc; }

    /**
     * Get the accumulator value
     * 
     * @return The accumulator value
     */
    uint8_t getA() const { return a; }

    /**
     * Get the X register value
     * 
     * @return The X register value
     */
    uint8_t getX() const { return x; }

    /**
     * Get the Y register value
     * 
     * @return The Y register value
     */
    uint8_t getY() const { return y; }

    /**
     * Get the status register value
     * 
     * @return The status register value
     */
    uint8_t getStatus() const { return status; }

    /**
     * Get the stack pointer value
     * 
     * @return The stack pointer value
     */
    uint8_t getSP() const { return sp; }

    /**
     * Get the total instruction count
     * 
     * @return The number of instructions executed
     */
    uint32_t getInstructionCount() const { return instructions; }

    /**
     * Get the total clock ticks
     * 
     * @return The number of clock ticks executed
     */
    uint32_t getClockTicks() const { return clockTicks; }

    /**
     * Set the program counter value
     * 
     * @param value The new program counter value
     */
    void setPC(uint16_t value) { pc = value; }

    /**
     * Set the accumulator value
     * 
     * @param value The new accumulator value
     */
    void setA(uint8_t value) { a = value; }

    /**
     * Set the X register value
     * 
     * @param value The new X register value
     */
    void setX(uint8_t value) { x = value; }

    /**
     * Set the Y register value
     * 
     * @param value The new Y register value
     */
    void setY(uint8_t value) { y = value; }

    /**
     * Set the status register value
     * 
     * @param value The new status register value
     */
    void setStatus(uint8_t value) { status = value; }

    /**
     * Set the stack pointer value
     * 
     * @param value The new stack pointer value
     */
    void setSP(uint8_t value) { sp = value; }

private:
    // CPU Registers
    uint16_t pc;            // Program Counter
    uint8_t sp;             // Stack Pointer
    uint8_t a;              // Accumulator
    uint8_t x;              // X Register
    uint8_t y;              // Y Register
    uint8_t status;         // Status Register

    // Emulator State
    uint32_t instructions;  // Instruction count
    uint32_t clockTicks;    // Total clock ticks
    uint32_t clockGoal;     // Target clock ticks for current exec
    uint16_t ea;            // Effective address
    uint16_t relAddr;       // Relative address for branch instructions
    uint16_t value;         // Value from memory
    uint16_t result;        // Result of operation
    uint8_t opcode;         // Current opcode
    bool penaltyOp;         // Page boundary penalty for certain operations
    bool penaltyAddr;       // Page boundary penalty for memory access

    // Memory Access
    std::function<uint8_t(uint16_t)> read;
    std::function<void(uint16_t, uint8_t)> write;

    // Hook for external function
    std::optional<std::function<void()>> hook;

    // Constants
    static constexpr uint8_t FLAG_CARRY     = 0x01;
    static constexpr uint8_t FLAG_ZERO      = 0x02;
    static constexpr uint8_t FLAG_INTERRUPT = 0x04;
    static constexpr uint8_t FLAG_DECIMAL   = 0x08;
    static constexpr uint8_t FLAG_BREAK     = 0x10;
    static constexpr uint8_t FLAG_CONSTANT  = 0x20;
    static constexpr uint8_t FLAG_OVERFLOW  = 0x40;
    static constexpr uint8_t FLAG_SIGN      = 0x80;
    static constexpr uint16_t BASE_STACK    = 0x0100;

    // Stack operations
    void push16(uint16_t value);
    void push8(uint8_t value);
    uint16_t pull16();
    uint8_t pull8();
    uint16_t read16(uint16_t addr);

    // Helper functions
    void saveAccum(uint16_t val) { a = static_cast<uint8_t>(val & 0xFF); }

    // Status flag management
    void setCarry() { status |= FLAG_CARRY; }
    void clearCarry() { status &= ~FLAG_CARRY; }
    void setZero() { status |= FLAG_ZERO; }
    void clearZero() { status &= ~FLAG_ZERO; }
    void setInterrupt() { status |= FLAG_INTERRUPT; }
    void clearInterrupt() { status &= ~FLAG_INTERRUPT; }
    void setDecimal() { status |= FLAG_DECIMAL; }
    void clearDecimal() { status &= ~FLAG_DECIMAL; }
    void setOverflow() { status |= FLAG_OVERFLOW; }
    void clearOverflow() { status &= ~FLAG_OVERFLOW; }
    void setSign() { status |= FLAG_SIGN; }
    void clearSign() { status &= ~FLAG_SIGN; }

    // Flag calculations
    void zeroCalc(uint16_t val) {
        if ((val & 0xFF) == 0) setZero(); else clearZero();
    }
    
    void signCalc(uint16_t val) {
        if ((val & 0x80) != 0) setSign(); else clearSign();
    }
    
    void carryCalc(uint16_t val) {
        if ((val & 0xFF00) != 0) setCarry(); else clearCarry();
    }
    
    void overflowCalc(uint16_t n, uint16_t m, uint16_t o) {
        if (((n ^ m) & (n ^ o) & 0x80) != 0) setOverflow(); else clearOverflow();
    }

    // Memory access
    uint16_t getValue();
    uint16_t getValue16();
    void putValue(uint16_t val);

    // Addressing mode handlers
    void addrImplied();
    void addrAccumulator();
    void addrImmediate();
    void addrZeroPage();
    void addrZeroPageX();
    void addrZeroPageY();
    void addrRelative();
    void addrAbsolute();
    void addrAbsoluteX();
    void addrAbsoluteY();
    void addrIndirect();
    void addrIndirectX();
    void addrIndirectY();

    // Instruction handlers
    void opADC();
    void opAND();
    void opASL();
    void opBCC();
    void opBCS();
    void opBEQ();
    void opBIT();
    void opBMI();
    void opBNE();
    void opBPL();
    void opBRK();
    void opBVC();
    void opBVS();
    void opCLC();
    void opCLD();
    void opCLI();
    void opCLV();
    void opCMP();
    void opCPX();
    void opCPY();
    void opDEC();
    void opDEX();
    void opDEY();
    void opEOR();
    void opINC();
    void opINX();
    void opINY();
    void opJMP();
    void opJSR();
    void opLDA();
    void opLDX();
    void opLDY();
    void opLSR();
    void opNOP();
    void opORA();
    void opPHA();
    void opPHP();
    void opPLA();
    void opPLP();
    void opROL();
    void opROR();
    void opRTI();
    void opRTS();
    void opSBC();
    void opSEC();
    void opSED();
    void opSEI();
    void opSTA();
    void opSTX();
    void opSTY();
    void opTAX();
    void opTAY();
    void opTSX();
    void opTXA();
    void opTXS();
    void opTYA();

    // Undocumented instructions
    void opLAX();
    void opSAX();
    void opDCP();
    void opISB();
    void opSLO();
    void opRLA();
    void opSRE();
    void opRRA();

    // Tables for instruction execution
    using AddrModeFn = void (Fake6502::*)();
    using InstructionFn = void (Fake6502::*)();
    
    static const AddrModeFn addrModeTable[256];
    static const InstructionFn instructionTable[256];
    static const uint8_t tickTable[256];
};

#endif // FAKE6502_HPP
