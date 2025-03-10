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
#include "logger.hpp"

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
    std::cout << "  -d, --debug             Enable debug logging\n";
    std::cout << "  -e, --extended-log      Enable extended log info (timestamps, log level)\n";
    std::cout << "  -h, --help              Show this help\n";
}

int main(int argc, char* argv[]) {
    // Initialize logger outside the try block so it's available throughout the function
    Logger& logger = getLogger();
    
    try {
        // Parse command line arguments
        std::string input_file;
        std::string output_file;
        std::string output_format = "asm";
        bool verbose = false;
        bool debug_logging = false;
        bool extended_log_info = false;
        int timeout = 300;  // 5 minutes
        
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            
            if (arg == "-f" || arg == "--file") {
                if (i + 1 < argc) {
                    input_file = argv[i + 1];
                    i++;
                } else {
                    logger.error("Missing input file");
                    return 1;
                }
            } else if (arg == "-o" || arg == "--output") {
                if (i + 1 < argc) {
                    output_file = argv[i + 1];
                    i++;
                } else {
                    logger.error("Missing output file");
                    return 1;
                }
            } else if (arg == "-format" || arg == "--output-format") {
                if (i + 1 < argc) {
                    output_format = argv[i + 1];
                    i++;
                } else {
                    logger.error("Missing format");
                    return 1;
                }
            } else if (arg == "-v" || arg == "--verbose") {
                verbose = true;
            } else if (arg == "-d" || arg == "--debug") {
                debug_logging = true;
            } else if (arg == "-e" || arg == "--extended-log") {
                extended_log_info = true;
            } else if (arg == "-t" || arg == "--timeout") {
                if (i + 1 < argc) {
                    timeout = std::stoi(argv[i + 1]);
                    i++;
                } else {
                    logger.error("Missing timeout value");
                    return 1;
                }
            } else if (arg == "-h" || arg == "--help") {
                printUsage();
                return 0;
            } else {
                logger.error("Unknown option: " + arg);
                return 1;
            }
        }
        
        // Configure logger based on command line options
        if (debug_logging) {
            logger.setLevel(Logger::DEBUG);
        }
        
        logger.setExtendedInfo(extended_log_info);
        logger.debug("Logger initialized with debug level: " + 
                    Logger::levelToString(logger.getLevel()) + 
                    ", extended info: " + (extended_log_info ? "enabled" : "disabled"));
        
        // Check required arguments
        if (input_file.empty()) {
            logger.error("Input file is required");
            logger.error("Use --help for usage information");
            return 1;
        }
        
        // Parse specification
        logger.info("Parsing specification from: " + input_file);
        logger.debug("Creating parser instance");
        PhaistosParser<uint16_t> parser;
        logger.debug("Starting to parse input file");
        OptimizationSpec<uint16_t> spec = parser.parse(input_file);
        logger.debug("Specification parsing completed successfully");
        
        if (verbose) {
            logger.info("Optimization goal: " + 
                       std::string(spec.goal == OptimizationSpec<uint16_t>::SIZE ? "size" : "speed"));
        }
        
        // Setup optimizer with progress listener
        logger.debug("Creating optimizer with parsed specification");
        Optimizer<uint16_t> optimizer(spec);
        ConsoleProgressListener listener;
        if (verbose) {
            logger.debug("Setting up progress listener for verbose output");
            optimizer.setProgressListener(&listener);
        }
        
        // Run optimization
        logger.info("Starting optimization (timeout: " + std::to_string(timeout) + " seconds)...");
        auto start_time = std::chrono::steady_clock::now();
        
        logger.debug("Calling optimizer.optimize() method");
        std::vector<uint8_t> solution = optimizer.optimize(timeout);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
        
        logger.info("Optimization completed in " + std::to_string(duration) + " seconds.");
        logger.debug("Solution size: " + std::to_string(solution.size()) + " bytes");
        
        if (solution.empty()) {
            logger.error("No valid solution found");
            return 1;
        }
        
        // Format the solution
        logger.debug("Creating solution formatter");
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
            logger.error("Unknown output format: " + output_format);
            return 1;
        }
        
        logger.debug("Formatting solution as " + output_format);
        std::string formatted = formatter.format(solution, format);
        
        // Add statistics
        std::string stats = formatter.getStatistics(solution);
        logger.info(stats);
        
        // Output the solution
        if (output_file.empty()) {
            // Output to stdout
            logger.info("\nOptimized solution:");
            // We use cout directly here since we want to show the formatted solution 
            // exactly as is without any logger formatting
            std::cout << formatted;
        } else {
            // Output to file
            logger.debug("Writing solution to file: " + output_file);
            std::ofstream out(output_file);
            if (!out) {
                logger.error("Failed to open output file: " + output_file);
                return 1;
            }
            out << formatted;
            out.close();
            
            logger.info("Solution written to: " + output_file);
        }
    } catch (const std::exception& e) {
        logger.error(std::string("Exception caught: ") + e.what());
        return 1;
    }
    
    logger.debug("Program completed successfully");
    return 0;
}
