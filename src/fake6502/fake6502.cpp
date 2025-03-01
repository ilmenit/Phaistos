#include "fake6502.hpp"

// Define NES_CPU if needed (uncomment for NES compatibility)
// #define NES_CPU

// Constructor
Fake6502::Fake6502(
    std::function<uint8_t(uint16_t)> memoryRead,
    std::function<void(uint16_t, uint8_t)> memoryWrite
) : read(std::move(memoryRead)),
    write(std::move(memoryWrite)),
    pc(0), sp(0), a(0), x(0), y(0), status(0),
    instructions(0), clockTicks(0), clockGoal(0),
    ea(0), relAddr(0), value(0), result(0), opcode(0),
    penaltyOp(false), penaltyAddr(false),
    hook(std::nullopt) {
}

// Stack operations
void Fake6502::push16(uint16_t value) {
    write(BASE_STACK + sp, (value >> 8) & 0xFF);
    write(BASE_STACK + ((sp - 1) & 0xFF), value & 0xFF);
    sp -= 2;
}

void Fake6502::push8(uint8_t value) {
    write(BASE_STACK + sp--, value);
}

uint16_t Fake6502::pull16() {
    uint16_t temp16 = read(BASE_STACK + ((sp + 1) & 0xFF)) | 
                     (static_cast<uint16_t>(read(BASE_STACK + ((sp + 2) & 0xFF))) << 8);
    sp += 2;
    return temp16;
}

uint8_t Fake6502::pull8() {
    return read(BASE_STACK + ++sp);
}

uint16_t Fake6502::read16(uint16_t addr) {
    return static_cast<uint16_t>(read(addr)) | 
          (static_cast<uint16_t>(read(addr + 1)) << 8);
}

// Reset the CPU
void Fake6502::reset() {
    // Replicate original reset behavior with dummy reads
    read(0x00ff);
    read(0x00ff);
    read(0x00ff);
    read(0x0100);
    read(0x01ff);
    read(0x01fe);
    pc = read16(0xfffc);
    sp = 0xfd;
    status = FLAG_CONSTANT | FLAG_INTERRUPT;
    a = 0;
    x = 0;
    y = 0;
    clockTicks = 0;
    instructions = 0;
}

// Execute for a specified number of clock cycles
uint32_t Fake6502::exec(uint32_t tickCount) {
    clockGoal = tickCount;
    clockTicks = 0;
    
    while (clockTicks < clockGoal) {
        opcode = read(pc++);
        status |= FLAG_CONSTANT;
        penaltyOp = false;
        penaltyAddr = false;
        
        // Execute the instruction
        (this->*addrModeTable[opcode])();
        (this->*instructionTable[opcode])();
        
        clockTicks += tickTable[opcode];
        if (penaltyOp && penaltyAddr) {
            clockTicks++;
        }
        
        instructions++;
        
        if (hook) {
            (*hook)();
        }
    }
    
    return clockTicks;
}

// Execute a single instruction
uint32_t Fake6502::step() {
    opcode = read(pc++);
    status |= FLAG_CONSTANT;
    
    penaltyOp = false;
    penaltyAddr = false;
    uint32_t stepTicks = 0;
    
    // Execute the instruction
    (this->*addrModeTable[opcode])();
    (this->*instructionTable[opcode])();
    
    stepTicks = tickTable[opcode];
    if (penaltyOp && penaltyAddr) {
        stepTicks++;
    }
    
    clockTicks += stepTicks;
    instructions++;
    
    if (hook) {
        (*hook)();
    }
    
    return stepTicks;
}

// Trigger an IRQ
void Fake6502::irq() {
    if ((status & FLAG_INTERRUPT) == 0) {
        push16(pc);
        push8(status & ~FLAG_BREAK);
        status |= FLAG_INTERRUPT;
        pc = read16(0xfffe);
    }
}

// Trigger an NMI
void Fake6502::nmi() {
    push16(pc);
    push8(status & ~FLAG_BREAK);
    status |= FLAG_INTERRUPT;
    pc = read16(0xfffa);
}

// Set hook function
void Fake6502::setHook(std::optional<std::function<void()>> newHook) {
    hook = std::move(newHook);
}

// Memory access functions
uint16_t Fake6502::getValue() {
    if (addrModeTable[opcode] == &Fake6502::addrAccumulator) {
        return a;
    } else {
        return read(ea);
    }
}

uint16_t Fake6502::getValue16() {
    return read(ea) | (static_cast<uint16_t>(read(ea + 1)) << 8);
}

void Fake6502::putValue(uint16_t val) {
    if (addrModeTable[opcode] == &Fake6502::addrAccumulator) {
        a = static_cast<uint8_t>(val & 0xFF);
    } else {
        write(ea, val & 0xFF);
    }
}

// Addressing mode implementations
void Fake6502::addrImplied() {
    // Implied addressing - no additional bytes needed
}

void Fake6502::addrAccumulator() {
    // Accumulator addressing - operation works on accumulator
}

void Fake6502::addrImmediate() {
    // Immediate addressing - use the next byte as the value
    ea = pc++;
}

void Fake6502::addrZeroPage() {
    // Zero page addressing - use the next byte as an address in the zero page
    ea = read(pc++);
}

void Fake6502::addrZeroPageX() {
    // Zero page,X addressing - add X to the next byte, wrap around in zero page
    ea = (read(pc++) + x) & 0xFF;
}

void Fake6502::addrZeroPageY() {
    // Zero page,Y addressing - add Y to the next byte, wrap around in zero page
    ea = (read(pc++) + y) & 0xFF;
}

void Fake6502::addrRelative() {
    // Relative addressing for branch instructions, sign-extended 8-bit offset
    relAddr = read(pc++);
    if (relAddr & 0x80) {
        relAddr |= 0xFF00;
    }
}

void Fake6502::addrAbsolute() {
    // Absolute addressing - next two bytes are the target address
    ea = read(pc) | (static_cast<uint16_t>(read(pc + 1)) << 8);
    pc += 2;
}

void Fake6502::addrAbsoluteX() {
    // Absolute,X addressing - add X to the next two bytes
    uint16_t startPage = read(pc) | (static_cast<uint16_t>(read(pc + 1)) << 8);
    uint16_t startPageHigh = startPage & 0xFF00;
    ea = startPage + x;
    
    // Add a cycle if page boundary is crossed
    if (startPageHigh != (ea & 0xFF00)) {
        penaltyAddr = true;
    }
    
    pc += 2;
}

void Fake6502::addrAbsoluteY() {
    // Absolute,Y addressing - add Y to the next two bytes
    uint16_t startPage = read(pc) | (static_cast<uint16_t>(read(pc + 1)) << 8);
    uint16_t startPageHigh = startPage & 0xFF00;
    ea = startPage + y;
    
    // Add a cycle if page boundary is crossed
    if (startPageHigh != (ea & 0xFF00)) {
        penaltyAddr = true;
    }
    
    pc += 2;
}

void Fake6502::addrIndirect() {
    // Indirect addressing - use the next two bytes as a pointer to the target
    uint16_t pointer = read(pc) | (static_cast<uint16_t>(read(pc + 1)) << 8);
    
    // Replicate the 6502 page boundary bug
    uint16_t pointer2 = (pointer & 0xFF00) | ((pointer + 1) & 0x00FF);
    
    ea = read(pointer) | (static_cast<uint16_t>(read(pointer2)) << 8);
    pc += 2;
}

void Fake6502::addrIndirectX() {
    // Pre-indexed indirect addressing (indirect,X)
    uint16_t zp = (read(pc++) + x) & 0xFF;
    ea = read(zp) | (static_cast<uint16_t>(read((zp + 1) & 0xFF)) << 8);
}

void Fake6502::addrIndirectY() {
    // Post-indexed indirect addressing (indirect),Y
    uint16_t zp = read(pc++);
    uint16_t pointer = read(zp) | (static_cast<uint16_t>(read((zp + 1) & 0xFF)) << 8);
    uint16_t startPageHigh = pointer & 0xFF00;
    ea = pointer + y;
    
    // Add a cycle if page boundary is crossed
    if (startPageHigh != (ea & 0xFF00)) {
        penaltyAddr = true;
    }
}

// Instruction implementations
void Fake6502::opADC() {
    penaltyOp = true;
    
#ifndef NES_CPU
    if (status & FLAG_DECIMAL) {
        // BCD arithmetic for standard 6502
        uint16_t AL, A, resultDec;
        A = a;
        value = getValue();
        resultDec = A + value + (status & FLAG_CARRY);
        
        // BCD adjustment per MOS 6502 datasheet
        AL = (A & 0x0F) + (value & 0x0F) + (status & FLAG_CARRY);
        if (AL >= 0x0A) {
            AL = ((AL + 0x06) & 0x0F) + 0x10;
        }
        
        A = (A & 0xF0) + (value & 0xF0) + AL;
        if (A & 0x80) {
            setSign();
        } else {
            clearSign();
        }
        
        if (A >= 0xA0) {
            A += 0x60;
        }
        
        result = A;
        
        if (A & 0xff80) {
            setOverflow();
        } else {
            clearOverflow();
        }
        
        if (A >= 0x100) {
            setCarry();
        } else {
            clearCarry();
        }
        
        zeroCalc(resultDec);
    } else
#endif
    {
        // Binary arithmetic
        value = getValue();
        result = a + value + (status & FLAG_CARRY);
        
        carryCalc(result);
        zeroCalc(result);
        overflowCalc(result, a, value);
        signCalc(result);
    }
    
    saveAccum(result);
}

void Fake6502::opAND() {
    penaltyOp = true;
    value = getValue();
    result = a & value;
    
    zeroCalc(result);
    signCalc(result);
    
    saveAccum(result);
}

void Fake6502::opASL() {
    value = getValue();
    result = value << 1;
    
    carryCalc(result);
    zeroCalc(result);
    signCalc(result);
    
    putValue(result);
}

void Fake6502::opBCC() {
    if ((status & FLAG_CARRY) == 0) {
        uint16_t oldPc = pc;
        pc += relAddr;
        
        if ((oldPc & 0xFF00) != (pc & 0xFF00)) {
            clockTicks += 2;  // Page boundary crossed
        } else {
            clockTicks++;
        }
    }
}

void Fake6502::opBCS() {
    if ((status & FLAG_CARRY) != 0) {
        uint16_t oldPc = pc;
        pc += relAddr;
        
        if ((oldPc & 0xFF00) != (pc & 0xFF00)) {
            clockTicks += 2;  // Page boundary crossed
        } else {
            clockTicks++;
        }
    }
}

void Fake6502::opBEQ() {
    if ((status & FLAG_ZERO) != 0) {
        uint16_t oldPc = pc;
        pc += relAddr;
        
        if ((oldPc & 0xFF00) != (pc & 0xFF00)) {
            clockTicks += 2;  // Page boundary crossed
        } else {
            clockTicks++;
        }
    }
}

void Fake6502::opBIT() {
    value = getValue();
    result = a & value;
    
    zeroCalc(result);
    status = (status & 0x3F) | (value & 0xC0);  // Copy bits 6 and 7 from value
}

void Fake6502::opBMI() {
    if ((status & FLAG_SIGN) != 0) {
        uint16_t oldPc = pc;
        pc += relAddr;
        
        if ((oldPc & 0xFF00) != (pc & 0xFF00)) {
            clockTicks += 2;  // Page boundary crossed
        } else {
            clockTicks++;
        }
    }
}

void Fake6502::opBNE() {
    if ((status & FLAG_ZERO) == 0) {
        uint16_t oldPc = pc;
        pc += relAddr;
        
        if ((oldPc & 0xFF00) != (pc & 0xFF00)) {
            clockTicks += 2;  // Page boundary crossed
        } else {
            clockTicks++;
        }
    }
}

void Fake6502::opBPL() {
    if ((status & FLAG_SIGN) == 0) {
        uint16_t oldPc = pc;
        pc += relAddr;
        
        if ((oldPc & 0xFF00) != (pc & 0xFF00)) {
            clockTicks += 2;  // Page boundary crossed
        } else {
            clockTicks++;
        }
    }
}

void Fake6502::opBRK() {
    pc++;
    push16(pc);
    push8(status | FLAG_BREAK);
    setInterrupt();
    pc = read16(0xFFFE);
}

void Fake6502::opBVC() {
    if ((status & FLAG_OVERFLOW) == 0) {
        uint16_t oldPc = pc;
        pc += relAddr;
        
        if ((oldPc & 0xFF00) != (pc & 0xFF00)) {
            clockTicks += 2;  // Page boundary crossed
        } else {
            clockTicks++;
        }
    }
}

void Fake6502::opBVS() {
    if ((status & FLAG_OVERFLOW) != 0) {
        uint16_t oldPc = pc;
        pc += relAddr;
        
        if ((oldPc & 0xFF00) != (pc & 0xFF00)) {
            clockTicks += 2;  // Page boundary crossed
        } else {
            clockTicks++;
        }
    }
}

void Fake6502::opCLC() {
    clearCarry();
}

void Fake6502::opCLD() {
    clearDecimal();
}

void Fake6502::opCLI() {
    clearInterrupt();
}

void Fake6502::opCLV() {
    clearOverflow();
}

void Fake6502::opCMP() {
    penaltyOp = true;
    value = getValue();
    result = a - value;
    
    if (a >= (value & 0xFF)) {
        setCarry();
    } else {
        clearCarry();
    }
    
    if (a == (value & 0xFF)) {
        setZero();
    } else {
        clearZero();
    }
    
    signCalc(result);
}

void Fake6502::opCPX() {
    value = getValue();
    result = x - value;
    
    if (x >= (value & 0xFF)) {
        setCarry();
    } else {
        clearCarry();
    }
    
    if (x == (value & 0xFF)) {
        setZero();
    } else {
        clearZero();
    }
    
    signCalc(result);
}

void Fake6502::opCPY() {
    value = getValue();
    result = y - value;
    
    if (y >= (value & 0xFF)) {
        setCarry();
    } else {
        clearCarry();
    }
    
    if (y == (value & 0xFF)) {
        setZero();
    } else {
        clearZero();
    }
    
    signCalc(result);
}

void Fake6502::opDEC() {
    value = getValue();
    result = value - 1;
    
    zeroCalc(result);
    signCalc(result);
    
    putValue(result);
}

void Fake6502::opDEX() {
    x--;
    
    zeroCalc(x);
    signCalc(x);
}

void Fake6502::opDEY() {
    y--;
    
    zeroCalc(y);
    signCalc(y);
}

void Fake6502::opEOR() {
    penaltyOp = true;
    value = getValue();
    result = a ^ value;
    
    zeroCalc(result);
    signCalc(result);
    
    saveAccum(result);
}

void Fake6502::opINC() {
    value = getValue();
    result = value + 1;
    
    zeroCalc(result);
    signCalc(result);
    
    putValue(result);
}

void Fake6502::opINX() {
    x++;
    
    zeroCalc(x);
    signCalc(x);
}

void Fake6502::opINY() {
    y++;
    
    zeroCalc(y);
    signCalc(y);
}

void Fake6502::opJMP() {
    pc = ea;
}

void Fake6502::opJSR() {
    push16(pc - 1);
    pc = ea;
}

void Fake6502::opLDA() {
    penaltyOp = true;
    value = getValue();
    a = value & 0xFF;
    
    zeroCalc(a);
    signCalc(a);
}

void Fake6502::opLDX() {
    penaltyOp = true;
    value = getValue();
    x = value & 0xFF;
    
    zeroCalc(x);
    signCalc(x);
}

void Fake6502::opLDY() {
    penaltyOp = true;
    value = getValue();
    y = value & 0xFF;
    
    zeroCalc(y);
    signCalc(y);
}

void Fake6502::opLSR() {
    value = getValue();
    
    if (value & 0x01) {
        setCarry();
    } else {
        clearCarry();
    }
    
    result = value >> 1;
    
    zeroCalc(result);
    signCalc(result);
    
    putValue(result);
}

void Fake6502::opNOP() {
    // Handle different NOP variants with cycle penalties
    switch (opcode) {
        case 0x1C:
        case 0x3C:
        case 0x5C:
        case 0x7C:
        case 0xDC:
        case 0xFC:
            penaltyOp = true;
            break;
    }
}

void Fake6502::opORA() {
    penaltyOp = true;
    value = getValue();
    result = a | value;
    
    zeroCalc(result);
    signCalc(result);
    
    saveAccum(result);
}

void Fake6502::opPHA() {
    push8(a);
}

void Fake6502::opPHP() {
    push8(status | FLAG_BREAK);
}

void Fake6502::opPLA() {
    a = pull8();
    
    zeroCalc(a);
    signCalc(a);
}

void Fake6502::opPLP() {
    status = pull8() | FLAG_CONSTANT;
}

void Fake6502::opROL() {
    value = getValue();
    result = (value << 1) | (status & FLAG_CARRY);
    
    carryCalc(result);
    zeroCalc(result);
    signCalc(result);
    
    putValue(result);
}

void Fake6502::opROR() {
    value = getValue();
    result = (value >> 1) | ((status & FLAG_CARRY) << 7);
    
    if (value & 0x01) {
        setCarry();
    } else {
        clearCarry();
    }
    
    zeroCalc(result);
    signCalc(result);
    
    putValue(result);
}

void Fake6502::opRTI() {
    status = pull8();
    value = pull16();
    pc = value;
}

void Fake6502::opRTS() {
    value = pull16();
    pc = value + 1;
}

void Fake6502::opSBC() {
    penaltyOp = true;
    
#ifndef NES_CPU
    if (status & FLAG_DECIMAL) {
        // BCD arithmetic for standard 6502
        uint16_t resultDec, A, AL, B, C;
        A = a;
        C = (status & FLAG_CARRY);
        value = getValue();
        B = value;
        value = value ^ 0xFF;  // One's complement for subtraction
        
        resultDec = A + value + C;
        
        // Standard checks
        carryCalc(resultDec);
        overflowCalc(resultDec, a, value);
        signCalc(resultDec);
        zeroCalc(resultDec);
        
        // NMOS-specific BCD adjustment
        AL = (A & 0x0F) - (B & 0x0F) + C - 1;
        if (AL & 0x8000) {
            AL = ((AL - 0x06) & 0x0F) - 0x10;
        }
        
        A = (A & 0xF0) - (B & 0xF0) + AL;
        if (A & 0x8000) {
            A = A - 0x60;
        }
        
        result = A;
    } else
#endif
    {
        value = getValue() ^ 0xFF;  // One's complement for subtraction
        result = a + value + (status & FLAG_CARRY);
        
        carryCalc(result);
        zeroCalc(result);
        overflowCalc(result, a, value);
        signCalc(result);
    }
    
    saveAccum(result);
}

void Fake6502::opSEC() {
    setCarry();
}

void Fake6502::opSED() {
    setDecimal();
}

void Fake6502::opSEI() {
    setInterrupt();
}

void Fake6502::opSTA() {
    putValue(a);
}

void Fake6502::opSTX() {
    putValue(x);
}

void Fake6502::opSTY() {
    putValue(y);
}

void Fake6502::opTAX() {
    x = a;
    
    zeroCalc(x);
    signCalc(x);
}

void Fake6502::opTAY() {
    y = a;
    
    zeroCalc(y);
    signCalc(y);
}

void Fake6502::opTSX() {
    x = sp;
    
    zeroCalc(x);
    signCalc(x);
}

void Fake6502::opTXA() {
    a = x;
    
    zeroCalc(a);
    signCalc(a);
}

void Fake6502::opTXS() {
    sp = x;
}

void Fake6502::opTYA() {
    a = y;
    
    zeroCalc(a);
    signCalc(a);
}

// Undocumented instructions
void Fake6502::opLAX() {
    // Implements both LDA and LDX
    penaltyOp = true;
    value = getValue();
    a = x = value & 0xFF;
    
    zeroCalc(a);
    signCalc(a);
}

void Fake6502::opSAX() {
    // Store A & X
    putValue(a & x);
    if (penaltyOp && penaltyAddr) {
        clockTicks--;
    }
}

void Fake6502::opDCP() {
    // DEC followed by CMP
    opDEC();
    opCMP();
    if (penaltyOp && penaltyAddr) {
        clockTicks--;
    }
}

void Fake6502::opISB() {
    // INC followed by SBC
    opINC();
    opSBC();
    if (penaltyOp && penaltyAddr) {
        clockTicks--;
    }
}

void Fake6502::opSLO() {
    // ASL followed by ORA
    opASL();
    opORA();
    if (penaltyOp && penaltyAddr) {
        clockTicks--;
    }
}

void Fake6502::opRLA() {
    // ROL followed by AND
    opROL();
    opAND();
    if (penaltyOp && penaltyAddr) {
        clockTicks--;
    }
}

void Fake6502::opSRE() {
    // LSR followed by EOR
    opLSR();
    opEOR();
    if (penaltyOp && penaltyAddr) {
        clockTicks--;
    }
}

void Fake6502::opRRA() {
    // ROR followed by ADC
    opROR();
    opADC();
    if (penaltyOp && penaltyAddr) {
        clockTicks--;
    }
}

// Lookup tables
const Fake6502::AddrModeFn Fake6502::addrModeTable[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */  &Fake6502::addrImplied, &Fake6502::addrIndirectX, &Fake6502::addrImplied, &Fake6502::addrIndirectX, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrImplied, &Fake6502::addrImmediate, &Fake6502::addrAccumulator, &Fake6502::addrImmediate, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, /* 0 */
/* 1 */  &Fake6502::addrRelative, &Fake6502::addrIndirectY, &Fake6502::addrImplied, &Fake6502::addrIndirectY, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, /* 1 */
/* 2 */  &Fake6502::addrAbsolute, &Fake6502::addrIndirectX, &Fake6502::addrImplied, &Fake6502::addrIndirectX, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrImplied, &Fake6502::addrImmediate, &Fake6502::addrAccumulator, &Fake6502::addrImmediate, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, /* 2 */
/* 3 */  &Fake6502::addrRelative, &Fake6502::addrIndirectY, &Fake6502::addrImplied, &Fake6502::addrIndirectY, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, /* 3 */
/* 4 */  &Fake6502::addrImplied, &Fake6502::addrIndirectX, &Fake6502::addrImplied, &Fake6502::addrIndirectX, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrImplied, &Fake6502::addrImmediate, &Fake6502::addrAccumulator, &Fake6502::addrImmediate, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, /* 4 */
/* 5 */  &Fake6502::addrRelative, &Fake6502::addrIndirectY, &Fake6502::addrImplied, &Fake6502::addrIndirectY, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, /* 5 */
/* 6 */  &Fake6502::addrImplied, &Fake6502::addrIndirectX, &Fake6502::addrImplied, &Fake6502::addrIndirectX, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrImplied, &Fake6502::addrImmediate, &Fake6502::addrAccumulator, &Fake6502::addrImmediate, &Fake6502::addrIndirect, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, /* 6 */
/* 7 */  &Fake6502::addrRelative, &Fake6502::addrIndirectY, &Fake6502::addrImplied, &Fake6502::addrIndirectY, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, /* 7 */
/* 8 */  &Fake6502::addrImmediate, &Fake6502::addrIndirectX, &Fake6502::addrImmediate, &Fake6502::addrIndirectX, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrImplied, &Fake6502::addrImmediate, &Fake6502::addrImplied, &Fake6502::addrImmediate, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, /* 8 */
/* 9 */  &Fake6502::addrRelative, &Fake6502::addrIndirectY, &Fake6502::addrImplied, &Fake6502::addrIndirectY, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageY, &Fake6502::addrZeroPageY, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteY, &Fake6502::addrAbsoluteY, /* 9 */
/* A */  &Fake6502::addrImmediate, &Fake6502::addrIndirectX, &Fake6502::addrImmediate, &Fake6502::addrIndirectX, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrImplied, &Fake6502::addrImmediate, &Fake6502::addrImplied, &Fake6502::addrImmediate, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, /* A */
/* B */  &Fake6502::addrRelative, &Fake6502::addrIndirectY, &Fake6502::addrImplied, &Fake6502::addrIndirectY, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageY, &Fake6502::addrZeroPageY, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteY, &Fake6502::addrAbsoluteY, /* B */
/* C */  &Fake6502::addrImmediate, &Fake6502::addrIndirectX, &Fake6502::addrImmediate, &Fake6502::addrIndirectX, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrImplied, &Fake6502::addrImmediate, &Fake6502::addrImplied, &Fake6502::addrImmediate, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, /* C */
/* D */  &Fake6502::addrRelative, &Fake6502::addrIndirectY, &Fake6502::addrImplied, &Fake6502::addrIndirectY, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, /* D */
/* E */  &Fake6502::addrImmediate, &Fake6502::addrIndirectX, &Fake6502::addrImmediate, &Fake6502::addrIndirectX, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrZeroPage, &Fake6502::addrImplied, &Fake6502::addrImmediate, &Fake6502::addrImplied, &Fake6502::addrImmediate, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, &Fake6502::addrAbsolute, /* E */
/* F */  &Fake6502::addrRelative, &Fake6502::addrIndirectY, &Fake6502::addrImplied, &Fake6502::addrIndirectY, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrZeroPageX, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrImplied, &Fake6502::addrAbsoluteY, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX, &Fake6502::addrAbsoluteX  /* F */
};

const Fake6502::InstructionFn Fake6502::instructionTable[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |      */
/* 0 */  &Fake6502::opBRK, &Fake6502::opORA, &Fake6502::opNOP, &Fake6502::opSLO, &Fake6502::opNOP, &Fake6502::opORA, &Fake6502::opASL, &Fake6502::opSLO, &Fake6502::opPHP, &Fake6502::opORA, &Fake6502::opASL, &Fake6502::opNOP, &Fake6502::opNOP, &Fake6502::opORA, &Fake6502::opASL, &Fake6502::opSLO, /* 0 */
/* 1 */  &Fake6502::opBPL, &Fake6502::opORA, &Fake6502::opNOP, &Fake6502::opSLO, &Fake6502::opNOP, &Fake6502::opORA, &Fake6502::opASL, &Fake6502::opSLO, &Fake6502::opCLC, &Fake6502::opORA, &Fake6502::opNOP, &Fake6502::opSLO, &Fake6502::opNOP, &Fake6502::opORA, &Fake6502::opASL, &Fake6502::opSLO, /* 1 */
/* 2 */  &Fake6502::opJSR, &Fake6502::opAND, &Fake6502::opNOP, &Fake6502::opRLA, &Fake6502::opBIT, &Fake6502::opAND, &Fake6502::opROL, &Fake6502::opRLA, &Fake6502::opPLP, &Fake6502::opAND, &Fake6502::opROL, &Fake6502::opNOP, &Fake6502::opBIT, &Fake6502::opAND, &Fake6502::opROL, &Fake6502::opRLA, /* 2 */
/* 3 */  &Fake6502::opBMI, &Fake6502::opAND, &Fake6502::opNOP, &Fake6502::opRLA, &Fake6502::opNOP, &Fake6502::opAND, &Fake6502::opROL, &Fake6502::opRLA, &Fake6502::opSEC, &Fake6502::opAND, &Fake6502::opNOP, &Fake6502::opRLA, &Fake6502::opNOP, &Fake6502::opAND, &Fake6502::opROL, &Fake6502::opRLA, /* 3 */
/* 4 */  &Fake6502::opRTI, &Fake6502::opEOR, &Fake6502::opNOP, &Fake6502::opSRE, &Fake6502::opNOP, &Fake6502::opEOR, &Fake6502::opLSR, &Fake6502::opSRE, &Fake6502::opPHA, &Fake6502::opEOR, &Fake6502::opLSR, &Fake6502::opNOP, &Fake6502::opJMP, &Fake6502::opEOR, &Fake6502::opLSR, &Fake6502::opSRE, /* 4 */
/* 5 */  &Fake6502::opBVC, &Fake6502::opEOR, &Fake6502::opNOP, &Fake6502::opSRE, &Fake6502::opNOP, &Fake6502::opEOR, &Fake6502::opLSR, &Fake6502::opSRE, &Fake6502::opCLI, &Fake6502::opEOR, &Fake6502::opNOP, &Fake6502::opSRE, &Fake6502::opNOP, &Fake6502::opEOR, &Fake6502::opLSR, &Fake6502::opSRE, /* 5 */
/* 6 */  &Fake6502::opRTS, &Fake6502::opADC, &Fake6502::opNOP, &Fake6502::opRRA, &Fake6502::opNOP, &Fake6502::opADC, &Fake6502::opROR, &Fake6502::opRRA, &Fake6502::opPLA, &Fake6502::opADC, &Fake6502::opROR, &Fake6502::opNOP, &Fake6502::opJMP, &Fake6502::opADC, &Fake6502::opROR, &Fake6502::opRRA, /* 6 */
/* 7 */  &Fake6502::opBVS, &Fake6502::opADC, &Fake6502::opNOP, &Fake6502::opRRA, &Fake6502::opNOP, &Fake6502::opADC, &Fake6502::opROR, &Fake6502::opRRA, &Fake6502::opSEI, &Fake6502::opADC, &Fake6502::opNOP, &Fake6502::opRRA, &Fake6502::opNOP, &Fake6502::opADC, &Fake6502::opROR, &Fake6502::opRRA, /* 7 */
/* 8 */  &Fake6502::opNOP, &Fake6502::opSTA, &Fake6502::opNOP, &Fake6502::opSAX, &Fake6502::opSTY, &Fake6502::opSTA, &Fake6502::opSTX, &Fake6502::opSAX, &Fake6502::opDEY, &Fake6502::opNOP, &Fake6502::opTXA, &Fake6502::opNOP, &Fake6502::opSTY, &Fake6502::opSTA, &Fake6502::opSTX, &Fake6502::opSAX, /* 8 */
/* 9 */  &Fake6502::opBCC, &Fake6502::opSTA, &Fake6502::opNOP, &Fake6502::opNOP, &Fake6502::opSTY, &Fake6502::opSTA, &Fake6502::opSTX, &Fake6502::opSAX, &Fake6502::opTYA, &Fake6502::opSTA, &Fake6502::opTXS, &Fake6502::opNOP, &Fake6502::opNOP, &Fake6502::opSTA, &Fake6502::opNOP, &Fake6502::opNOP, /* 9 */
/* A */  &Fake6502::opLDY, &Fake6502::opLDA, &Fake6502::opLDX, &Fake6502::opLAX, &Fake6502::opLDY, &Fake6502::opLDA, &Fake6502::opLDX, &Fake6502::opLAX, &Fake6502::opTAY, &Fake6502::opLDA, &Fake6502::opTAX, &Fake6502::opNOP, &Fake6502::opLDY, &Fake6502::opLDA, &Fake6502::opLDX, &Fake6502::opLAX, /* A */
/* B */  &Fake6502::opBCS, &Fake6502::opLDA, &Fake6502::opNOP, &Fake6502::opLAX, &Fake6502::opLDY, &Fake6502::opLDA, &Fake6502::opLDX, &Fake6502::opLAX, &Fake6502::opCLV, &Fake6502::opLDA, &Fake6502::opTSX, &Fake6502::opLAX, &Fake6502::opLDY, &Fake6502::opLDA, &Fake6502::opLDX, &Fake6502::opLAX, /* B */
/* C */  &Fake6502::opCPY, &Fake6502::opCMP, &Fake6502::opNOP, &Fake6502::opDCP, &Fake6502::opCPY, &Fake6502::opCMP, &Fake6502::opDEC, &Fake6502::opDCP, &Fake6502::opINY, &Fake6502::opCMP, &Fake6502::opDEX, &Fake6502::opNOP, &Fake6502::opCPY, &Fake6502::opCMP, &Fake6502::opDEC, &Fake6502::opDCP, /* C */
/* D */  &Fake6502::opBNE, &Fake6502::opCMP, &Fake6502::opNOP, &Fake6502::opDCP, &Fake6502::opNOP, &Fake6502::opCMP, &Fake6502::opDEC, &Fake6502::opDCP, &Fake6502::opCLD, &Fake6502::opCMP, &Fake6502::opNOP, &Fake6502::opDCP, &Fake6502::opNOP, &Fake6502::opCMP, &Fake6502::opDEC, &Fake6502::opDCP, /* D */
/* E */  &Fake6502::opCPX, &Fake6502::opSBC, &Fake6502::opNOP, &Fake6502::opISB, &Fake6502::opCPX, &Fake6502::opSBC, &Fake6502::opINC, &Fake6502::opISB, &Fake6502::opINX, &Fake6502::opSBC, &Fake6502::opNOP, &Fake6502::opSBC, &Fake6502::opCPX, &Fake6502::opSBC, &Fake6502::opINC, &Fake6502::opISB, /* E */
/* F */  &Fake6502::opBEQ, &Fake6502::opSBC, &Fake6502::opNOP, &Fake6502::opISB, &Fake6502::opNOP, &Fake6502::opSBC, &Fake6502::opINC, &Fake6502::opISB, &Fake6502::opSED, &Fake6502::opSBC, &Fake6502::opNOP, &Fake6502::opISB, &Fake6502::opNOP, &Fake6502::opSBC, &Fake6502::opINC, &Fake6502::opISB  /* F */
};

const uint8_t Fake6502::tickTable[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */      7,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    4,    4,    6,    6,  /* 0 */
/* 1 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 1 */
/* 2 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    4,    4,    6,    6,  /* 2 */
/* 3 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 3 */
/* 4 */      6,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    3,    4,    6,    6,  /* 4 */
/* 5 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 5 */
/* 6 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    5,    4,    6,    6,  /* 6 */
/* 7 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 7 */
/* 8 */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* 8 */
/* 9 */      2,    6,    2,    6,    4,    4,    4,    4,    2,    5,    2,    5,    5,    5,    5,    5,  /* 9 */
/* A */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* A */
/* B */      2,    5,    2,    5,    4,    4,    4,    4,    2,    4,    2,    4,    4,    4,    4,    4,  /* B */
/* C */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* C */
/* D */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* D */
/* E */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* E */
/* F */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7   /* F */
};