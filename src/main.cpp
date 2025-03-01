/**
 * @file main.cpp
 * @brief Command-line interface for Phaistos
 */
#include "common.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "solution_formatter.hpp"

using namespace phaistos;

/**
 * @brief Print usage information
 */
static void printUsage() {
    std::cout << "Phaistos 6502 Optimizer\n";
    std::cout << "Usage: phaistos [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -f, --file <file>       Input specification file (required)\n";
    std::cout << "  -o, --output <file>     Output file (default: stdout)\n";
    std::cout << "  -format, --output-format <format>  Output format (default: asm)\n";
    std::cout << "                          Formats: asm, bin, c, basic\n";
    std::cout << "  -v, --verbose           Enable verbose output\n";
    std::cout << "  -t, --timeout <seconds> Set timeout (default: 300)\n";
    std::cout << "  -h, --help              Show this help\n";
}

int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        std::string input_file;
        std::string output_file;
        std::string output_format = "asm";
        bool verbose = false;
        int timeout = 300;  // 5 minutes
        
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            
            if (arg == "-f" || arg == "--file") {
                if (i + 1 < argc) {
                    input_file = argv[i + 1];
                    i++;
                } else {
                    std::cerr << "Error: Missing input file\n";
                    return 1;
                }
            } else if (arg == "-o" || arg == "--output") {
                if (i + 1 < argc) {
                    output_file = argv[i + 1];
                    i++;
                } else {
                    std::cerr << "Error: Missing output file\n";
                    return 1;
                }
            } else if (arg == "-format" || arg == "--output-format") {
                if (i + 1 < argc) {
                    output_format = argv[i + 1];
                    i++;
                } else {
                    std::cerr << "Error: Missing format\n";
                    return 1;
                }
            } else if (arg == "-v" || arg == "--verbose") {
                verbose = true;
            } else if (arg == "-t" || arg == "--timeout") {
                if (i + 1 < argc) {
                    timeout = std::stoi(argv[i + 1]);
                    i++;
                } else {
                    std::cerr << "Error: Missing timeout value\n";
                    return 1;
                }
            } else if (arg == "-h" || arg == "--help") {
                printUsage();
                return 0;
            } else {
                std::cerr << "Error: Unknown option: " << arg << "\n";
                return 1;
            }
        }
        
        // Check required arguments
        if (input_file.empty()) {
            std::cerr << "Error: Input file is required\n";
            std::cerr << "Use --help for usage information\n";
            return 1;
        }
        
        // Parse specification
        std::cout << "Parsing specification from: " << input_file << std::endl;
        PhaistosParser<uint16_t> parser;
        OptimizationSpec<uint16_t> spec = parser.parse(input_file);
        
        if (verbose) {
            std::cout << "Optimization goal: " 
                      << (spec.goal == OptimizationSpec<uint16_t>::SIZE ? "size" : "speed")
                      << std::endl;
        }
        
        // Setup optimizer with progress listener
        Optimizer<uint16_t> optimizer(spec);
        ConsoleProgressListener listener;
        if (verbose) {
            optimizer.setProgressListener(&listener);
        }
        
        // Run optimization
        std::cout << "Starting optimization (timeout: " << timeout << " seconds)..." << std::endl;
        auto start_time = std::chrono::steady_clock::now();
        
        std::vector<uint8_t> solution = optimizer.optimize(timeout);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
        
        std::cout << "Optimization completed in " << duration << " seconds." << std::endl;
        
        if (solution.empty()) {
            std::cerr << "Error: No valid solution found\n";
            return 1;
        }
        
        // Format the solution
        SolutionFormatter formatter;
        SolutionFormatter::Format format;
        
        if (output_format == "asm") {
            format = SolutionFormatter::ASSEMBLY;
        } else if (output_format == "bin") {
            format = SolutionFormatter::BINARY;
        } else if (output_format == "c") {
            format = SolutionFormatter::C_ARRAY;
        } else if (output_format == "basic") {
            format = SolutionFormatter::BASIC_DATA;
        } else {
            std::cerr << "Error: Unknown output format: " << output_format << "\n";
            return 1;
        }
        
        std::string formatted = formatter.format(solution, format);
        
        // Add statistics
        std::string stats = formatter.getStatistics(solution);
        std::cout << stats;
        
        // Output the solution
        if (output_file.empty()) {
            // Output to stdout
            std::cout << "\nOptimized solution:\n";
            std::cout << formatted;
        } else {
            // Output to file
            std::ofstream out(output_file);
            if (!out) {
                std::cerr << "Error: Failed to open output file: " << output_file << "\n";
                return 1;
            }
            out << formatted;
            out.close();
            
            std::cout << "Solution written to: " << output_file << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
