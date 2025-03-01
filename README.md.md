# Phaistos: 6502 Code Optimizer

Phaistos is a specialized tool for optimizing 6502 assembly code. Unlike traditional compilers or optimizers, Phaistos uses a constraint-based approach to discover mathematically optimal solutions for given specifications.

## Features

- Find the shortest possible 6502 code sequence for a specific task
- Find the fastest execution sequence for time-critical code
- Support for detailed input/output specifications
- Accurate cycle-counting and emulation
- Flexible optimization for both size and speed
- Code synthesis based on memory transformations

## Installation

### Prerequisites

- C++17 compatible compiler
- CMake 3.12 or higher
- (Optional) Doxygen for documentation

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/phaistos.git
cd phaistos

# Create a build directory and navigate to it
mkdir build && cd build

# Configure and build
cmake ..
make

# Install (optional)
make install
```

## Usage

Phaistos uses a custom file format (`.pha`) to define optimization problems:

```bash
phaistos -f my_optimization.pha -o optimized.asm
```

Options:
- `-f, --file`: Input specification file (required)
- `-o, --output`: Output file (default: stdout)
- `-format, --output-format`: Output format - asm, bin, c, basic (default: asm)
- `-v, --verbose`: Enable verbose output
- `-t, --timeout`: Set timeout in seconds (default: 300)
- `-h, --help`: Show help

## Example Specifications

### 16-bit Increment

```
# inc16.pha
OPTIMIZE_FOR: size

MEMORY_IN:
  0x80: 0x? 0x?  ; 16-bit value in zero page
  END

MEMORY_OUT:
  0x80: 0x? 0x?  ; Incremented 16-bit value
  END
```

### Code Synthesis

```
# double.pha
OPTIMIZE:
  0x2000:
    END  ; Empty block - generate code here

RUN: 0x2000

MEMORY_IN:
  0x3000:
    0x00 0x01 0x02 0x03 0x04
    END

MEMORY_OUT:
  0x3000:
    0x00 0x02 0x04 0x06 0x08  ; Multiplied by 2
    END
```

## File Format

Phaistos uses a custom specification format with:

- `OPTIMIZE_FOR`: Global directive for optimization goal (size or speed)
- `CPU_IN/OUT`: CPU register state before/after
- `FLAGS_IN/OUT`: CPU flags before/after
- `MEMORY_IN/OUT`: Memory contents before/after
- `OPTIMIZE`/`OPTIMIZE_RO`: Code blocks to optimize
- `RUN`: Execution start address
- `END`: Memory block terminator

Values can be:
- `EXACT`: Specific values (like `0x42`)
- `ANY`: Any value (specified with `?`)
- `SAME`: Preserved input value

## License

[MIT License](LICENSE)

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
