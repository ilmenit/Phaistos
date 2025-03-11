# Phaistos: High-Level Design Document

## 1. Introduction

Phaistos is a specialized tool designed to find the optimal implementation of 6502 assembly code sequences. Unlike traditional compilers or optimizers, Phaistos uses a constraint-based approach to discover the mathematically optimal solution for a given specification, whether optimizing for code size or execution speed.

### 1.1 Purpose

The primary purpose of Phaistos is to:

1. Find the shortest possible sequence of 6502 instructions that transforms a given initial state to a desired output state
2. Find the fastest sequence of instructions for the same transformation
3. Support constraint-based optimization with parameterized inputs and outputs
4. Handle the nuances of 6502 programming, including memory access, flag management, and cycle-accurate timing

### 1.2 Target Audience

- Retro game developers working on resource-constrained 6502 systems
- Demo scene programmers seeking maximum performance
- Compiler developers looking to generate optimal code
- Retrocomputing enthusiasts optimizing classic software

### 1.3 Core Features

- **State-based Specification**: Define initial and target CPU/memory states
- **Value Types (EXACT/ANY/SAME/EQU)**: Flexible specification of values and constraints
- **Cycle-accurate Optimization**: True cycle-counting for speed optimizations
- **Memory-aware Processing**: Handles memory reads, writes, and self-modifying code
- **Flag-accurate Execution**: Full support for all CPU status flags
- **Comprehensive Verification**: Ensures correctness across all valid inputs

## 2. System Architecture

### 2.1 High-Level Architecture

```
┌───────────────┐      ┌───────────────┐      ┌───────────────┐
│ Specification │─────▶│ Sequence      │─────▶│ Verification  │
│  Parser       │      │ Generator     │      │  Engine       │
└───────────────┘      └───────────────┘      └───────────────┘
                              │                      │
                              ▼                      ▼
                      ┌───────────────┐      ┌───────────────┐
                      │ CPU/Memory    │◀─────│ Optimization  │
                      │ Emulator      │      │  Cache        │
                      └───────────────┘      └───────────────┘
                              │                      │
                              ▼                      ▼
                      ┌───────────────┐      ┌───────────────┐
                      │ Transformation│◀────▶│ Solution      │
                      │  Analyzer     │      │  Formatter    │
                      └───────────────┘      └───────────────┘
```

### 2.2 Key Components

1. **Specification Parser**: Reads and validates input specifications
2. **Sequence Generator**: Produces candidate instruction sequences
3. **CPU/Memory Emulator**: Accurately executes 6502 code sequences
4. **Verification Engine**: Checks if solutions meet requirements
5. **Optimization Cache**: Stores optimal sequences for reuse
6. **Solution Formatter**: Outputs the optimized solution

### 2.3 Component Interactions

1. The **Specification Parser** reads a .pha file and constructs a structured representation of the initial state, expected final state, and optimization goal.

2. The **Sequence Generator** produces candidate instruction sequences, starting with the shortest possible ones and incrementally increasing in length.

3. The **CPU/Memory Emulator** executes each candidate sequence on the initial state to determine the resulting state.

4. The **Verification Engine** checks if the resulting state matches the required output state as defined in the specification.

5. The **Optimization Cache** stores known-optimal instruction sequences for specific state transformations, enabling reuse and composition.

6. The **Solution Formatter** converts the optimized instruction sequence into the desired output format.

## 3. Input Specification Format

### 3.1 File Format (.pha)

Phaistos uses a custom text file format with the `.pha` extension. The format is designed to be:

- Human-readable and editable
- Close to assembly language notation where appropriate
- Explicit in specifying both EXACT and ANY (don't care) values
- Flexible in representation (hex, binary, decimal)

### 3.2 Basic Structure

```
; Phaistos Specification

OPTIMIZE_FOR: size  ; or 'speed'

; Initial CPU state
CPU_IN:
  A: 0x42      ; Exact value
  X: ?         ; Any value (can also use ANY or ??)
  Y: 0x00      ; Exact value
  SP: ?        ; Any value

; CPU flags (separate for clarity)
FLAGS_IN:
  C: 0         ; Exact: carry clear
  Z: ?         ; Any: zero flag can be anything
  I: 1         ; Exact: interrupt disable set
  D: 0         ; Exact: decimal mode off
  B: ?         ; Any
  V: ?         ; Any
  N: ?         ; Any

; Input memory regions
MEMORY_IN:
  ; Format: ADDRESS: [byte values]
  0x1000: 0xA9 0x? 0x85 0x20    ; LDA #? STA $20
  0x2000: :4 0x?                ; Four ANY bytes
  0x0080: 0x01 0x02 0x03 0x04   ; Data in zero page

; Code to be optimized
OPTIMIZE:
  0x3000: 
    0xA9 0x?   ; LDA #?
    0x85 0x20   ; STA $20
    END         ; End marker for this code block

RUN: 0x1000

; Expected CPU state after execution
CPU_OUT:
  A: ?         ; Don't care
  X: 0x42      ; Must be exactly 0x42
  Y: ?         ; Don't care
  SP: ?        ; Don't care

; Expected CPU flags after execution
FLAGS_OUT:
  C: 1         ; Carry must be set
  Z: 0         ; Zero must be clear
  I: ?         ; Don't care
  D: 0         ; Decimal mode must be off
  B: ?         ; Don't care
  V: ?         ; Don't care
  N: ?         ; Don't care

; Expected memory state after execution
MEMORY_OUT:
  ; Only specify regions that matter
  0x0040: 0x42 0x?     ; First byte must be 0x42, second can be anything
  0x1000: :4 0x?       ; Don't care about original code area
```

### 3.3 Value Representation

- Hexadecimal: `0x42`, `$42`, or `42h`
- Binary: `0b10101010` or `%10101010`
- Decimal: `42`
- ANY value: `?`, `??`, `ANY`, or with prefix like `0x?` or `$?`
- Repeated values: `:N value` (e.g., `:4 0x??` for 4 bytes of any value)

All value types work with the repeat notation:
```
:4 0x42      ; 4 bytes of exact value 0x42
:4 0x??      ; 4 bytes of ANY value
:4 SAME      ; 4 bytes that must match their input values
:4 EQU       ; 4 bytes that must match original code output
```

### 3.4 Meaning of Value Types

The specification includes four value types that have different meanings in input versus output contexts:

1. **EXACT**: Specifies a precise value
   - In input: The value must be exactly as specified
   - In output: The value must end up exactly as specified

2. **ANY**: Represents any possible value
   - In input: The value can be anything (will be tested with various values)
   - In output: Don't care what value ends up here (not verified)

3. **SAME**: Preserves the input value (only valid in output)
   - In output: The value must be preserved from input, whatever it was

4. **EQU**: Functionally equivalent to original code output (only valid in output)
   - In output: The value must match what the original code in the OPTIMIZE section would produce given the same inputs

> **Important Note**: EXACT and ANY can be used in both input and output contexts, while SAME and EQU can ONLY be used in output contexts. Using SAME or EQU in an input context will result in a specification error.

These value types enable several common patterns:

- **EXACT input → EXACT output**: Transform one specific value to another
- **EXACT input → ANY output**: "Don't care" about the output value
- **EXACT input → EQU output**: Produce output equivalent to what the original code would produce
- **ANY input → EXACT output**: Produce a specific output regardless of input
- **ANY input → SAME output**: Preserve the value regardless of what it was
- **ANY input → EQU output**: Transform the input exactly as the original code would
- **ANY input → ANY output**: Value can be used as a temporary/scratch

The SAME type is particularly important for registers or memory that must be preserved, regardless of their initial values.

The EQU type is critical for ensuring that the optimized code produces the same functional output as the original code, without having to define complex relationships between input and output values.

### 3.5 Unspecified Values

Any CPU registers, flags, or memory locations that are not explicitly specified in the input or output sections are treated as having the ANY (?) value. This means:

- For `CPU_IN` and `FLAGS_IN`: Unspecified registers or flags can have any value
- For `CPU_OUT` and `FLAGS_OUT`: Unspecified registers or flags can end up with any value (don't care)
- For `MEMORY_IN` and `MEMORY_OUT`: Only specified memory regions are considered relevant

This convention allows for concise specifications where only the relevant state needs to be defined. For example, if a particular optimization doesn't use or modify the Y register, it can be omitted from both the input and output specifications.

CPU_IN and CPU_OUT sections can be completely omitted if no CPU registers are relevant to the optimization problem.

### 3.6 END Marker

The `END` marker indicates the termination point of a code block definition. Important characteristics of the `END` marker include:

- `END` does not add any byte to memory
- `END` simply marks where the code definition stops
- `END` is only used for code blocks in OPTIMIZE sections, not for data in MEMORY_IN or MEMORY_OUT sections

### 3.7 RUN Directive

The `RUN` directive specifies the address where execution should begin:

```
RUN: 0x1000  ; Start execution at address 0x1000
```

Key characteristics of the `RUN` directive:

- Execution always starts at the specified address
- If multiple code blocks are defined, only the one at the RUN address (or reachable from it) will be executed
- For indirect optimization (e.g., optimizing a function called by another routine), `RUN` should point to the calling routine, not the code being optimized
- If `RUN` is not specified, execution defaults to the start of the first `OPTIMIZE` block

Here's an example with two code blocks:

```
OPTIMIZE:
  0x2000:
    0xA9 0x01   ; LDA #$01
    0xC9 0x00   ; CMP #$00
    0xF0 0x05   ; BEQ $2100 (Branch to second block)
    0xA9 0x02   ; LDA #$02
    0x85 0x20   ; STA $20
    0x4C 0x07 0x20 ; JMP $2007 (Skip second block)
    END
  
  0x2100:
    0xA9 0x03   ; LDA #$03
    0x85 0x20   ; STA $20
    0x4C 0x07 0x20 ; JMP $2007
    END
```

In this example, each `END` marker simply defines where its respective code block definition stops. The markers themselves don't affect program execution and aren't part of the code.

### 3.7 Optimization Directives

#### 3.7.1 Global Optimization Goal

The `OPTIMIZE_FOR` directive specifies the overall optimization goal:

```
OPTIMIZE_FOR: size   ; Find the smallest code
```

or

```
OPTIMIZE_FOR: speed  ; Find the fastest code
```

This directive controls whether Phaistos prioritizes minimizing code size or execution time when generating solutions.

#### 3.7.2 OPTIMIZE Directive

The `OPTIMIZE` directive marks a memory block that should be analyzed for optimization:

```
OPTIMIZE:
  0x1000: 
    0xA9 0x?   ; LDA #?
    0x85 0x20   ; STA $20
    0xA9 0x?   ; LDA #?
    0x85 0x20   ; STA $20
    END
```

This tells the optimizer to look for patterns and generate more efficient code. The `??` symbol indicates variable values that can be parameterized. The optimizer is allowed to generate self-modifying code if that would result in better performance.

#### 3.7.3 OPTIMIZE_RO Directive

The `OPTIMIZE_RO` directive specifically marks memory blocks that should be optimized as read-only (non-self-modifying) code:

```
OPTIMIZE_RO:
  0x1000: 
    0xA9 0x?   ; LDA #?
    0x85 0x20   ; STA $20
    0xA9 0x?   ; LDA #?
    0x85 0x20   ; STA $20
    END
```

This prevents the optimizer from generating self-modifying code, which may be more predictable in execution time but potentially less compact.

#### 3.7.4 Code Synthesis

For more advanced scenarios, an empty `OPTIMIZE` block can be used along with input/output memory states to generate code that performs a specific transformation:

```
OPTIMIZE:
  0x2000:
    END  ; No instructions - generate optimal code here

RUN: 0x2000

MEMORY_IN:
  0x3000:
    0x00 0x01 0x02 0x03 0x04 0x05 0x06

MEMORY_OUT:
  0x3000:
    0x00 0x02 0x04 0x06 0x08 0x0A 0x0C  ; Multiplied by 2
```

This instructs the system to synthesize the most efficient code to transform the memory from the input state to the output state, placing the generated code at address 0x2000.

## 4. Memory Management and I/O Handling

### 4.1 Memory Access Rules

Phaistos enforces strict memory access rules to ensure code correctness:

1. Code can only read from memory locations defined in `MEMORY_IN`
2. Code can only write to memory locations defined in `MEMORY_OUT`
3. Accessing undefined memory regions will terminate emulation with an error (therefore code is not equivalent)
4. Self-modifying code must only modify regions defined in both `MEMORY_IN` and `MEMORY_OUT`

### 4.2 Memory-Mapped I/O Handling

When working with memory-mapped hardware registers or I/O ports, the following limitations apply:

1. **Explicit Declaration Required**: All hardware registers must be defined in `MEMORY_OUT` to make them "writeable"
2. **Final Value Limitation**: Only the final value written to a hardware register is tracked
3. **Intermediate Values**: Multiple writes to the same register during execution are not tracked or verified
4. **Timing-Sensitive Operations**: Phaistos cannot optimize timing-sensitive register access patterns

Example of hardware register definition:

```
; Hardware registers
MEMORY_OUT:
  0xD000: 0x??  ; VIC-II sprite 0 X-position
  0xD001: 0x??  ; VIC-II sprite 0 Y-position
  0xD010: 0x??  ; VIC-II sprites X-position MSBs
  0xD015: 0x??  ; VIC-II sprite enable register
```

### 4.3 Side Effect Management

For memory locations with side effects when read or written:

1. If a location has side effects when written, it should be defined in `MEMORY_OUT`
2. If a location has side effects when read, it should be defined in `MEMORY_IN`
3. Phaistos cannot account for external state changes caused by these side effects
4. For hardware with complex state machines or timing requirements, manual verification may be needed

### 4.4 Memory Access Limitations

Phaistos has specific limitations when optimizing code that interacts with hardware:

1. **Sequential Register Writes**: Cannot track or optimize sequences of writes to the same register
2. **Read-Modify-Write**: Cannot optimize hardware registers requiring read-modify-write operations
3. **Timing Dependencies**: Cannot account for register values that depend on precise timing
4. **Interrupt Handling**: Cannot optimize code where hardware state might be modified by interrupts

These limitations should be carefully considered when optimizing code that interacts with hardware.

## 5. Optimization Algorithm

### 5.1 Core Optimization Approach

At a high level, Phaistos's optimization algorithm works as follows:

1. **Generate Test Cases**: For each ANY parameter in the initial state (CPU registers, flags, memory), generate concrete test values to create a set of test cases
2. **Generate**: Produce candidate instruction sequences in order of increasing length
3. **Execute**: For each candidate sequence, execute it on all test cases to determine its effect
4. **Verify**: Check if the sequence produces the correct output state for every test case
5. **Cache**: Store successful transformations for future reference
6. **Optimize**: Use the cache to find shorter/faster equivalents for subsequences

The first valid sequence found when optimizing for size is guaranteed to be optimal because sequences are generated in order of increasing length. 

For speed optimization, the process is more complex because cycle counts on the 6502 depend on runtime conditions:
- Page boundary crossing penalties (e.g., indexing across a page boundary)
- Branch taken vs. not taken
- Self-modifying code behavior

Therefore, when optimizing for speed, Phaistos:
1. Generates sequences by length
2. Calculates accurate cycle counts (including all conditional effects)
3. Tracks the fastest solution found so far
4. Continues searching until reaching a reasonable bound (e.g., best_solution_length + a small margin)

This approach finds highly optimized solutions for both size and speed, with a mathematical guarantee of optimality for size optimization.

The generation of test cases from ANY parameters is a critical step. Each ANY value in the input specification could take any of 256 possible values (for an 8-bit register or memory location). Testing all combinations would cause combinatorial explosion, so Phaistos uses strategic sampling with boundary values (0, 1, 0x7F, 0x80, 0xFF) and representative random values to ensure thorough verification while keeping the number of test cases manageable.

### 5.2 Sequence Generation Strategy

Sequences are generated systematically:

1. Start with sequences of length 1 (single instructions)
2. Test all valid opcodes and addressing modes
3. Incrementally increase sequence length
4. Apply pruning rules to avoid exploring obviously invalid sequences
5. Once a valid solution is found, limit the search to reasonable bounds (solution length + a small margin)

### 5.3 ANY Value Handling

When a specification includes ANY values (parameters that can take any value), Phaistos uses a strategic multi-level approach:

1. **Boundary Values**: First test critical edge cases (0, 1, 0x7F, 0x80, 0xFF)
2. **Representative Values**: Then test semi-randomly selected values from different ranges
3. **Exhaustive Verification (Optional)**: For final verification of promising solutions, test all 256 possible values

This approach provides a balance between thorough verification and computational efficiency:

- Early testing of boundary values quickly identifies most issues
- Representative sampling provides broader coverage
- Exhaustive verification can be used when needed for critical code

For equivalence checking, Phaistos uses a fail-fast approach, terminating verification as soon as any test case fails, while ensuring that equivalent states are fully validated across the necessary input space.

### 5.4 Caching and Reuse

Phaistos employs a transformation cache to optimize performance:

1. Each executed sequence is analyzed to determine its transformation effect
2. This transformation (input state → output state) is stored in the cache
3. When generating longer sequences, the cache is checked for known-optimal subsequences
4. If a more efficient equivalent is found for a subsequence, it's substituted

This approach enables compositional optimization, where knowledge about small sequences contributes to optimizing larger ones.

### 5.5 Verification Strategy

Verification ensures that a sequence correctly implements the required transformation:

1. The initial state is set up according to the specification
2. The sequence is executed on this state
3. The resulting state is compared against the required output state
4. For ANY input values, multiple test cases are generated and verified
5. For memory correctness, the system verifies that no unauthorized memory locations are modified
6. For EQU outputs, the original code is executed on the same inputs to generate reference outputs

When verifying code with EQU outputs, the system performs these additional steps:
1. Tracks all memory locations accessed by the original code
2. Ensures the optimized code only accesses the same memory locations
3. Records all outputs produced by the original code for comparison
4. Verifies that the optimized code produces identical outputs for all test cases

### 5.6 Algorithm Flow

```
function optimize(spec):
    // Create components
    generator = new SequenceGenerator()
    verifier = new VerificationEngine(spec)
    cache = new TransformationCache()
    
    // Configure generator
    generator.setMaxLength(32)  // Reasonable starting point
    
    // Track best solution
    best_solution = null
    best_metric = INFINITY
    
    // Main optimization loop
    while (candidate = generator.next()):
        // Try to optimize using cache
        optimized_candidate = optimizeWithCache(candidate, cache, spec)
        
        // Check if valid
        if (verifier.verify(optimized_candidate)):
            // Calculate size or cycles based on goal
            metric = (spec.goal == SIZE) ? 
                     getSize(optimized_candidate) : 
                     getCycles(optimized_candidate)
            
            // Update if better
            if (metric < best_metric):
                best_solution = optimized_candidate
                best_metric = metric
                
                // Limit search after finding solution
                generator.setMaxLength(best_solution.length + 4)
        
        // Add to cache for future use
        transformation = extractTransformation(optimized_candidate, spec)
        cache.add(transformation, optimized_candidate, getCycles(optimized_candidate))
    
    return best_solution
```

## 6. Key Implementation Considerations

### 6.1 Memory Representation

Phaistos uses a sparse memory model:

1. **Sparse Memory Model**: Only stores memory locations that are actually accessed
2. **Access Validation**: Ensures reads only come from memory areas defined in input, and writes only go to memory areas defined in output
3. **Error Detection**: Any access outside these defined memory areas is considered an error

This approach minimizes memory usage while ensuring all memory operations follow the specification constraints.

### 6.2 CPU Emulation

The CPU emulation layer is designed for flexibility and extensibility:

1. **Pluggable Architecture**: Supports integration with external emulation libraries
2. **Configurable Design**: Initially configured for 6502, but designed to support other CPUs (Z80, etc.)
3. **Cycle-Accurate**: Correctly accounts for all cycle costs, including conditional timing effects
4. **Deterministic**: Produces consistent results for given inputs
5. **Clear Interface**: Well-defined API allows for easy substitution of different CPU implementations

### 6.3 Performance Optimizations

Several techniques are employed to maximize performance:

1. **Instruction Caching**: Cache the results of executing individual instructions
2. **Transformation Memoization**: Remember the effect of instruction sequences
3. **Parallel Verification**: Test multiple input cases simultaneously
4. **Early Pruning**: Avoid generating and testing obviously invalid sequences
5. **Bounded Search**: Once a solution is found, limit the search space

## 7. Example Use Cases

### 7.1 Optimizing a 16-bit Increment

**Input (`inc16.pha`):**
```
OPTIMIZE_FOR: size

FLAGS_IN:
  D: 0         ; Decimal mode off

MEMORY_IN:
  0x80: 0x?? 0x??  ; 16-bit value in zero page

OPTIMIZE:
  0x1000:
    0x18        ; CLC
    0xA5 0x80   ; LDA $80
    0x69 0x01   ; ADC #$01
    0x85 0x80   ; STA $80
    0xA5 0x81   ; LDA $81
    0x69 0x00   ; ADC #$00
    0x85 0x81   ; STA $81
    END

RUN: 0x1000

FLAGS_OUT:
  D: SAME      ; Decimal mode should remain off

MEMORY_OUT:
  0x80: EQU EQU  ; 16-bit value must be incremented by 1
```

**Generated Solution:**
```
        INC $80
        BNE skip
        INC $81
skip:
```

### 7.2 Fast 8-bit Multiplication

**Input (`multiply.pha`):**
```
OPTIMIZE_FOR: speed

CPU_IN:
  A: ?  ; First operand
  X: ?  ; Second operand
  Y: 0x00

OPTIMIZE:
  0x1000:
    ; This algorithm multiplies A and X by repeated addition
    ; It performs A * X and stores result in $F1
    0xA8        ; TAY (preserve A)
    0xA9 0x00   ; LDA #0 (initialize result)
    0x86 0xF0   ; STX $F0 (store multiplier)
    0xC6 0xF0   ; DEC $F0 (adjust for loop logic)
multiply_loop:
    0x18        ; CLC
    0x65 0xF0   ; ADC $F0 (add X to accumulator each iteration)
    0xC6 0xF0   ; DEC $F0 (decrement counter)
    0x10 0xF9   ; BPL multiply_loop (loop until negative)
    0x85 0xF1   ; STA $F1 (store result low byte)
    0x98        ; TYA (restore original A)
    END

RUN: 0x1000

CPU_OUT:
  A: SAME       ; Preserve original value
  Y: SAME       ; Preserve original value

MEMORY_OUT:
  0xF1: EQU     ; Low byte of product - must match result from original code
```

### 7.3 Memory Block Copy

**Input (`memcopy.pha`):**
```
OPTIMIZE_FOR: size

CPU_IN:
  X: 0x00
  Y: 0x??  ; Length of copy (1-128 bytes)

MEMORY_IN:
  0x0200: :128 0x?  ; Source data (128 bytes)
  0x0300: :128 0x?  ; Destination (128 bytes)

OPTIMIZE:
  0x1000:
    0xB1 0xFC   ; LDA ($FC),Y - Source pointer at $FC,$FD
    0x91 0xFE   ; STA ($FE),Y - Destination pointer at $FE,$FF  
    0x88        ; DEY
    0x10 0xF9   ; BPL loop
    END

RUN: 0x1000

MEMORY_OUT:
  0x0300: :128 EQU  ; Copied data that matches what original code would produce
```

### 7.4 Code Optimization Example

**Input (`optimize_code.pha`):**
```
OPTIMIZE:
  0x1000:
    0xA9 0x00   ; LDA #$00
    0x18        ; CLC
    0x69 0x01   ; ADC #$01
    0x69 0x01   ; ADC #$01
    0x85 0x20   ; STA $20
    END

RUN: 0x1000

MEMORY_OUT:
  0x20: EQU     ; Result must match what original code produces (which is 2)
```

**Note**: We could also use `0x20: 0x02` here instead of `EQU` since we know the exact result will always be 2, but using `EQU` is more consistent with the pattern of having the optimizer match the original code's behavior.

### 7.5 Code Synthesis Example

**Input (`synthesize.pha`):**
```
MEMORY_IN:
  0x3000: 0x00 0x01 0x02 0x03 0x04

OPTIMIZE:
  0x2000:
    END  ; Empty block - generate code here

RUN: 0x2000

MEMORY_OUT:
  0x3000: 0x00 0x02 0x04 0x06 0x08  ; Values multiplied by 2
```

**Note**: In this code synthesis example, we use exact values (0x00, 0x02, etc.) rather than EQU because there is no original code to match - we're explicitly defining the desired transformation. This is appropriate for code synthesis where we're generating code from scratch to achieve a specific output.

### 7.6 Hardware Register Example

**Input (`sprite_setup.pha`):**
```
OPTIMIZE_FOR: size

MEMORY_IN:
  0x80: 0x?? 0x??  ; Sprite X,Y coordinates in zero page

OPTIMIZE:
  0x1000:
    0xA5 0x80   ; LDA $80
    0x8D 0x00 0xD0  ; STA $D000 (Sprite 0 X)
    0xA5 0x81   ; LDA $81
    0x8D 0x01 0xD0  ; STA $D001 (Sprite 0 Y)
    0xA9 0x01   ; LDA #$01
    0x8D 0x15 0xD0  ; STA $D015 (Sprite enable)
    END

RUN: 0x1000

MEMORY_OUT:
  0x80: SAME SAME  ; Coordinates should be preserved
  0xD000: EQU      ; Final X coordinate value
  0xD001: EQU      ; Final Y coordinate value
  0xD015: 0x01     ; Sprite 0 enabled
```

### 7.7 Example with Complex Calling Context

**Input (`context_optimize.pha`):**
```
MEMORY_IN:
  0x1000: ; Loop code calling the function we want to optimize
    0xA2 0x00   ; LDX #$00
  loop_start:
    0x20 0x20 0x10 ; JSR function_to_optimize
    0xE8        ; INX
    0xE0 0x10   ; CPX #$10
    0xD0 0xF8   ; BNE loop_start
    0x60        ; RTS
  
  0x2000: :256 0x??  ; Test data array

OPTIMIZE:
  0x1020: ; Function we want to optimize (16-bit increment)
    0x18        ; CLC
    0xA5 0x80   ; LDA $80
    0x69 0x01   ; ADC #$01
    0x85 0x80   ; STA $80
    0xA5 0x81   ; LDA $81
    0x69 0x00   ; ADC #$00
    0x85 0x81   ; STA $81
    0x60        ; RTS
    END
 
MEMORY_OUT:
  0x2000: :256 EQU  ; All test data must match what original code would produce
 
RUN: 0x1000  ; Start execution at the loop, not directly at the optimized function
```

## 8. Command Line Interface

### 8.1 Basic Usage

```
phaistos -f <input_file.pha> [-o <output_file>] [-format asm|bin|c|basic] [-v]
```

### 8.2 Options

- `-f, --file`: Input specification file (required)
- `-o, --output`: Output file (defaults to stdout)
- `-format, --output-format`: Output format (default: asm)
- `-v, --verbose`: Enable verbose output
- `-t, --timeout`: Set timeout in seconds (default: 300)
- `-h, --help`: Show help

### 8.3 Example Usage

```
# Basic optimization
phaistos -f double_byte.pha -o double_byte.asm

# Binary output
phaistos -f input_routine.pha -o routine.bin -format bin

# Verbose mode
phaistos -f complex_logic.pha -v

# With timeout
phaistos -f hard_problem.pha -t 600
```

## 9. Future Extensions

1. **Advanced Constraint System**: Support for algebraic relationships and complex constraints
2. **Multi-Path Optimization**: Optimize code with multiple execution paths
3. **Integration with Assemblers**: Direct integration with common 6502 assemblers
4. **Additional CPU Architectures**: Support for 65C02, 65816, Z80, etc.
5. **Graphical Interface**: Visual representation of optimization process
6. **Enhanced I/O Handling**: Better support for complex hardware interaction patterns
7. **Timing-Sensitive Optimization**: Support for hardware that requires specific timing patterns

## Appendix: File Format Specification

Detailed specification of the .pha file format:

1. **Directives**:
   - `OPTIMIZE_FOR`: Specifies optimization goal (`size` or `speed`)
   - `OPTIMIZE`: Marks code for optimization with self-modifying allowed
   - `OPTIMIZE_RO`: Marks code for read-only optimization (no self-modifying code)
   - `CPU_IN`: Defines initial CPU state
   - `FLAGS_IN`: Defines initial flag state
   - `MEMORY_IN`: Defines initial memory state
   - `CPU_OUT`: Defines expected CPU state
   - `FLAGS_OUT`: Defines expected flag state
   - `MEMORY_OUT`: Defines expected memory state
   - `END`: Marks the end of a code block (only used in OPTIMIZE sections)
   - `RUN`: Specifies the execution start address

2. **Value Types**:
   - `EXACT`: Specific value (default for numeric literals)
   - `ANY`: Any value (specified with `?`, `??`, or `ANY`)
   - `SAME`: Must match input value (only for outputs)
   - `EQU`: Must match what original code would produce (only for outputs)

3. **Value Formats**:
   - Hexadecimal: `0x42`, `$42`, or `42h`
   - Binary: `0b10101010` or `%10101010`
   - Decimal: `42`
   - ANY value: `?`, `??`, `ANY`, `0x?`, or `$?`
   - Repeated values: `:N value` (e.g., `:4 0x??` for 4 bytes of any value)

4. **Memory Specification**:
   - Horizontal: `ADDRESS: BYTE1 BYTE2 ...`
   - Vertical:
     ```
     ADDRESS:
       BYTE1 ; Optional comment
       BYTE2 ; Optional comment
     ```
   - Repeated: `ADDRESS: :N VALUE` (e.g., `0x2000: :256 0x??`)

5. **Optimization Blocks**:
   - Standard optimization: 
     ```
     OPTIMIZE:
       ADDRESS:
         INSTRUCTION1
         INSTRUCTION2
         END
     ```
   - Read-only optimization:
     ```
     OPTIMIZE_RO:
       ADDRESS:
         INSTRUCTION1
         INSTRUCTION2
         END
     ```
   - Code synthesis:
     ```
     OPTIMIZE:
       ADDRESS:
         END  ; Empty block for synthesized code
     ```
