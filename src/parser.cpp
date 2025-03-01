/**
 * @file parser.cpp
 * @brief Implementation of the PhaistosParser
 */
#include "parser.hpp"
#include <sstream>
#include <regex>

namespace phaistos {

template<typename AddressT>
OptimizationSpec<AddressT> PhaistosParser<AddressT>::parse(const std::string& filename) {
    OptimizationSpec<AddressT> spec;
    
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    int line_number = 0;
    std::string line;
    while (std::getline(file, line)) {
        line_number++;
        
        // Skip comments and empty lines
        if (line.empty() || line[0] == ';') {
            continue;
        }
        
        // Tokenize line
        std::vector<Token> tokens = tokenize(line, line_number);
        if (tokens.empty()) {
            continue;
        }
        
        // Parse directives
        if (tokens[0].type == TokenType::DIRECTIVE) {
            if (tokens[0].value == "OPTIMIZE_FOR") {
                parseOptimizationGoal(tokens, spec);
            } else if (tokens[0].value == "CPU_IN") {
                parseCPUState(file, spec.input_cpu);
            } else if (tokens[0].value == "FLAGS_IN") {
                parseFlagState(file, spec.input_flags);
            } else if (tokens[0].value == "MEMORY_IN") {
                parseMemoryRegions(file, spec.input_memory);
            } else if (tokens[0].value == "CPU_OUT") {
                parseCPUState(file, spec.output_cpu);
            } else if (tokens[0].value == "FLAGS_OUT") {
                parseFlagState(file, spec.output_flags);
            } else if (tokens[0].value == "MEMORY_OUT") {
                parseMemoryRegions(file, spec.output_memory);
            } else if (tokens[0].value == "RUN") {
                parseRunAddress(tokens, spec);
            } else if (tokens[0].value == "OPTIMIZE") {
                parseOptimizeBlock(file, spec, false);
            } else if (tokens[0].value == "OPTIMIZE_RO") {
                parseOptimizeBlock(file, spec, true);
            } else {
                throw std::runtime_error("Unknown directive: " + tokens[0].value + 
                                         " at line " + std::to_string(line_number));
            }
        }
    }
    
    return spec;
}

template<typename AddressT>
std::vector<typename PhaistosParser<AddressT>::Token> PhaistosParser<AddressT>::tokenize(
    const std::string& line, int line_number) {
    
    std::vector<Token> tokens;
    std::string current_token;
    TokenType current_type = TokenType::DIRECTIVE;
    int column = 0;
    
    // Handle common directives
    static const std::unordered_set<std::string> directives = {
        "OPTIMIZE_FOR", "CPU_IN", "FLAGS_IN", "MEMORY_IN", 
        "CPU_OUT", "FLAGS_OUT", "MEMORY_OUT", "RUN",
        "OPTIMIZE", "OPTIMIZE_RO", "END"
    };
    
    static const std::unordered_set<std::string> registers = {
        "A", "X", "Y", "SP", "PC"
    };
    
    static const std::unordered_set<std::string> flags = {
        "C", "Z", "I", "D", "B", "V", "N"
    };
    
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        column = i + 1;
        
        // Skip comments
        if (c == ';') {
            break;
        }
        
        // Handle special characters
        if (c == ':') {
            if (!current_token.empty()) {
                // Check if it's an address
                if (current_token.find("0x") == 0 || 
                    current_token.find("$") == 0 ||
                    std::all_of(current_token.begin(), current_token.end(), ::isxdigit)) {
                    tokens.emplace_back(TokenType::ADDRESS, current_token, line_number, column - current_token.size());
                } else if (directives.find(current_token) != directives.end()) {
                    tokens.emplace_back(TokenType::DIRECTIVE, current_token, line_number, column - current_token.size());
                } else if (registers.find(current_token) != registers.end()) {
                    tokens.emplace_back(TokenType::REGISTER, current_token, line_number, column - current_token.size());
                } else if (flags.find(current_token) != flags.end()) {
                    tokens.emplace_back(TokenType::FLAG, current_token, line_number, column - current_token.size());
                } else {
                    tokens.emplace_back(TokenType::VALUE, current_token, line_number, column - current_token.size());
                }
                current_token.clear();
            }
            tokens.emplace_back(TokenType::COLON, ":", line_number, column);
            continue;
        }
        
        if (c == '=') {
            if (!current_token.empty()) {
                if (registers.find(current_token) != registers.end()) {
                    tokens.emplace_back(TokenType::REGISTER, current_token, line_number, column - current_token.size());
                } else if (flags.find(current_token) != flags.end()) {
                    tokens.emplace_back(TokenType::FLAG, current_token, line_number, column - current_token.size());
                } else {
                    tokens.emplace_back(TokenType::VALUE, current_token, line_number, column - current_token.size());
                }
                current_token.clear();
            }
            tokens.emplace_back(TokenType::EQUALS, "=", line_number, column);
            continue;
        }
        
        // Handle whitespace
        if (std::isspace(c)) {
            if (!current_token.empty()) {
                if (current_token == "END") {
                    tokens.emplace_back(TokenType::END, current_token, line_number, column - current_token.size());
                } else if (current_token == "??") {
                    tokens.emplace_back(TokenType::ANY, current_token, line_number, column - current_token.size());
                } else if (current_token == "?" || current_token == "ANY") {
                    tokens.emplace_back(TokenType::ANY, current_token, line_number, column - current_token.size());
                } else if (current_token == "SAME") {
                    tokens.emplace_back(TokenType::SAME, current_token, line_number, column - current_token.size());
                } else if (directives.find(current_token) != directives.end()) {
                    tokens.emplace_back(TokenType::DIRECTIVE, current_token, line_number, column - current_token.size());
                } else if (registers.find(current_token) != registers.end()) {
                    tokens.emplace_back(TokenType::REGISTER, current_token, line_number, column - current_token.size());
                } else if (flags.find(current_token) != flags.end()) {
                    tokens.emplace_back(TokenType::FLAG, current_token, line_number, column - current_token.size());
                } else {
                    tokens.emplace_back(TokenType::VALUE, current_token, line_number, column - current_token.size());
                }
                current_token.clear();
            }
            continue;
        }
        
        // Accumulate token
        current_token += c;
    }
    
    // Handle last token
    if (!current_token.empty()) {
        if (current_token == "END") {
            tokens.emplace_back(TokenType::END, current_token, line_number, column - current_token.size());
        } else if (current_token == "??") {
            tokens.emplace_back(TokenType::ANY, current_token, line_number, column - current_token.size());
        } else if (current_token == "?" || current_token == "ANY") {
            tokens.emplace_back(TokenType::ANY, current_token, line_number, column - current_token.size());
        } else if (current_token == "SAME") {
            tokens.emplace_back(TokenType::SAME, current_token, line_number, column - current_token.size());
        } else if (directives.find(current_token) != directives.end()) {
            tokens.emplace_back(TokenType::DIRECTIVE, current_token, line_number, column - current_token.size());
        } else if (registers.find(current_token) != registers.end()) {
            tokens.emplace_back(TokenType::REGISTER, current_token, line_number, column - current_token.size());
        } else if (flags.find(current_token) != flags.end()) {
            tokens.emplace_back(TokenType::FLAG, current_token, line_number, column - current_token.size());
        } else {
            tokens.emplace_back(TokenType::VALUE, current_token, line_number, column - current_token.size());
        }
    }
    
    return tokens;
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseOptimizationGoal(
    const std::vector<Token>& tokens, 
    OptimizationSpec<AddressT>& spec) {
    
    if (tokens.size() < 3) {
        throw std::runtime_error("Invalid OPTIMIZE_FOR directive, expected OPTIMIZE_FOR: goal");
    }
    
    if (tokens[1].type != TokenType::COLON) {
        throw std::runtime_error("Expected ':' after OPTIMIZE_FOR");
    }
    
    std::string goal = tokens[2].value;
    std::transform(goal.begin(), goal.end(), goal.begin(), ::tolower);
    
    if (goal == "size") {
        spec.goal = OptimizationSpec<AddressT>::SIZE;
    } else if (goal == "speed") {
        spec.goal = OptimizationSpec<AddressT>::SPEED;
    } else {
        throw std::runtime_error("Invalid optimization goal: " + goal + ", expected 'size' or 'speed'");
    }
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseCPUState(
    std::istream& input, 
    typename OptimizationSpec<AddressT>::CPUState& state) {
    
    std::string line;
    
    while (std::getline(input, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == ';') {
            continue;
        }
        
        // Check for end of section
        if (line.find("CPU_") == 0 || 
            line.find("FLAGS_") == 0 || 
            line.find("MEMORY_") == 0 ||
            line.find("OPTIMIZE") == 0 ||
            line.find("RUN") == 0) {
            // Push back the line for the next directive
            input.seekg(-static_cast<int>(line.size()) - 1, std::ios_base::cur);
            break;
        }
        
        // Parse register assignments
        std::vector<Token> tokens = tokenize(line, 0);
        if (tokens.empty()) {
            continue;
        }
        
        if (tokens.size() < 3) {
            throw std::runtime_error("Invalid CPU register assignment: " + line);
        }
        
        if (tokens[0].type != TokenType::REGISTER) {
            throw std::runtime_error("Expected register name, got: " + tokens[0].value);
        }
        
        if (tokens[1].type != TokenType::COLON && tokens[1].type != TokenType::EQUALS) {
            throw std::runtime_error("Expected ':' or '=' after register, got: " + tokens[1].value);
        }
        
        std::string reg = tokens[0].value;
        Value value;
        
        if (tokens[2].type == TokenType::ANY) {
            value = Value::any();
        } else if (tokens[2].type == TokenType::SAME) {
            value = Value::same();
        } else {
            value = Value::parse(tokens[2].value);
        }
        
        if (reg == "A") {
            state.a = value;
        } else if (reg == "X") {
            state.x = value;
        } else if (reg == "Y") {
            state.y = value;
        } else if (reg == "SP") {
            state.sp = value;
        } else {
            throw std::runtime_error("Unknown register: " + reg);
        }
    }
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseFlagState(
    std::istream& input, 
    typename OptimizationSpec<AddressT>::FlagState& flags) {
    
    std::string line;
    
    while (std::getline(input, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == ';') {
            continue;
        }
        
        // Check for end of section
        if (line.find("CPU_") == 0 || 
            line.find("FLAGS_") == 0 || 
            line.find("MEMORY_") == 0 ||
            line.find("OPTIMIZE") == 0 ||
            line.find("RUN") == 0) {
            // Push back the line for the next directive
            input.seekg(-static_cast<int>(line.size()) - 1, std::ios_base::cur);
            break;
        }
        
        // Parse flag assignments
        std::vector<Token> tokens = tokenize(line, 0);
        if (tokens.empty()) {
            continue;
        }
        
        if (tokens.size() < 3) {
            throw std::runtime_error("Invalid CPU flag assignment: " + line);
        }
        
        if (tokens[0].type != TokenType::FLAG) {
            throw std::runtime_error("Expected flag name, got: " + tokens[0].value);
        }
        
        if (tokens[1].type != TokenType::COLON && tokens[1].type != TokenType::EQUALS) {
            throw std::runtime_error("Expected ':' or '=' after flag, got: " + tokens[1].value);
        }
        
        std::string flag = tokens[0].value;
        Value value;
        
        if (tokens[2].type == TokenType::ANY) {
            value = Value::any();
        } else if (tokens[2].type == TokenType::SAME) {
            value = Value::same();
        } else {
            value = Value::parse(tokens[2].value);
        }
        
        if (flag == "C") {
            flags.c = value;
        } else if (flag == "Z") {
            flags.z = value;
        } else if (flag == "I") {
            flags.i = value;
        } else if (flag == "D") {
            flags.d = value;
        } else if (flag == "B") {
            flags.b = value;
        } else if (flag == "V") {
            flags.v = value;
        } else if (flag == "N") {
            flags.n = value;
        } else {
            throw std::runtime_error("Unknown flag: " + flag);
        }
    }
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseMemoryRegions(
    std::istream& input, 
    std::vector<typename OptimizationSpec<AddressT>::MemoryRegion>& regions) {
    
    std::string line;
    
    while (std::getline(input, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == ';') {
            continue;
        }
        
        // Check for end of section
        if (line.find("CPU_") == 0 || 
            line.find("FLAGS_") == 0 || 
            line.find("MEMORY_") == 0 ||
            line.find("OPTIMIZE") == 0 ||
            line.find("RUN") == 0) {
            // Push back the line for the next directive
            input.seekg(-static_cast<int>(line.size()) - 1, std::ios_base::cur);
            break;
        }
        
        // Parse memory region
        std::vector<Token> tokens = tokenize(line, 0);
        if (tokens.empty()) {
            continue;
        }
        
        if (tokens[0].type == TokenType::ADDRESS) {
            if (tokens.size() < 2 || tokens[1].type != TokenType::COLON) {
                throw std::runtime_error("Invalid memory region format, expected ADDRESS: values");
            }
            
            if (tokens.size() > 2) {
                // Horizontal format
                auto region = parseHorizontalMemoryRegion(tokens);
                regions.push_back(region);
            } else {
                // Vertical format
                typename OptimizationSpec<AddressT>::MemoryRegion region;
                region.address = parseAddress(tokens[0]);
                
                // Parse values line by line
                bool end_found = false;
                while (std::getline(input, line) && !end_found) {
                    // Skip comments and empty lines
                    if (line.empty() || line[0] == ';') {
                        continue;
                    }
                    
                    // Parse line
                    tokens = tokenize(line, 0);
                    if (tokens.empty()) {
                        continue;
                    }
                    
                    // Check for next region or directive
                    if (tokens[0].type == TokenType::ADDRESS || 
                        tokens[0].type == TokenType::DIRECTIVE) {
                        // Push back the line for the next region/directive
                        input.seekg(-static_cast<int>(line.size()) - 1, std::ios_base::cur);
                        break;
                    }
                    
                    // Parse values
                    for (const auto& token : tokens) {
                        if (token.type == TokenType::VALUE) {
                            region.bytes.push_back(Value::parse(token.value));
                        } else if (token.type == TokenType::ANY) {
                            region.bytes.push_back(Value::any());
                        } else if (token.type == TokenType::SAME) {
                            region.bytes.push_back(Value::same());
                        }
                    }
                }
                
                if (!region.bytes.empty()) {
                    regions.push_back(region);
                }
            }
        } else {
            throw std::runtime_error("Invalid memory region format, expected ADDRESS: values");
        }
    }
}

template<typename AddressT>
typename OptimizationSpec<AddressT>::MemoryRegion 
PhaistosParser<AddressT>::parseHorizontalMemoryRegion(const std::vector<Token>& tokens) {
    typename OptimizationSpec<AddressT>::MemoryRegion region;
    region.address = parseAddress(tokens[0]);
    
    for (size_t i = 2; i < tokens.size(); i++) {
        if (tokens[i].type == TokenType::VALUE) {
            region.bytes.push_back(Value::parse(tokens[i].value));
        } else if (tokens[i].type == TokenType::ANY) {
            region.bytes.push_back(Value::any());
        } else if (tokens[i].type == TokenType::SAME) {
            region.bytes.push_back(Value::same());
        }
    }
    
    return region;
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseRunAddress(
    const std::vector<Token>& tokens, 
    OptimizationSpec<AddressT>& spec) {
    
    if (tokens.size() < 3) {
        throw std::runtime_error("Invalid RUN directive, expected RUN: address");
    }
    
    if (tokens[1].type != TokenType::COLON) {
        throw std::runtime_error("Expected ':' after RUN");
    }
    
    if (tokens[2].type != TokenType::ADDRESS && tokens[2].type != TokenType::VALUE) {
        throw std::runtime_error("Expected address after RUN:");
    }
    
    spec.run_address = parseAddress(tokens[2]);
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseOptimizeBlock(
    std::istream& input, 
    OptimizationSpec<AddressT>& spec,
    bool read_only) {
    
    std::string line;
    
    while (std::getline(input, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == ';') {
            continue;
        }
        
        // Parse line
        std::vector<Token> tokens = tokenize(line, 0);
        if (tokens.empty()) {
            continue;
        }
        
        // Check for end of section
        if (tokens[0].type == TokenType::DIRECTIVE && 
            (tokens[0].value == "CPU_IN" || 
             tokens[0].value == "FLAGS_IN" || 
             tokens[0].value == "MEMORY_IN" || 
             tokens[0].value == "CPU_OUT" || 
             tokens[0].value == "FLAGS_OUT" || 
             tokens[0].value == "MEMORY_OUT" ||
             tokens[0].value == "OPTIMIZE" ||
             tokens[0].value == "OPTIMIZE_RO" ||
             tokens[0].value == "RUN")) {
            // Push back the line for the next directive
            input.seekg(-static_cast<int>(line.size()) - 1, std::ios_base::cur);
            break;
        }
        
        // Handle address
        if (tokens[0].type == TokenType::ADDRESS) {
            if (tokens.size() < 2 || tokens[1].type != TokenType::COLON) {
                throw std::runtime_error("Invalid OPTIMIZE block format, expected ADDRESS: values");
            }
            
            typename OptimizationSpec<AddressT>::CodeBlock block;
            block.address = parseAddress(tokens[0]);
            block.type = read_only ? 
                        OptimizationSpec<AddressT>::CodeBlock::READ_ONLY : 
                        OptimizationSpec<AddressT>::CodeBlock::REGULAR;
            
            // Check if we have an empty block for code synthesis
            if (tokens.size() > 2 && tokens[2].type == TokenType::END) {
                // Empty block for code synthesis
                spec.code_blocks.push_back(block);
                continue;
            }
            
            // Parse block contents
            bool end_found = false;
            while (std::getline(input, line) && !end_found) {
                // Skip comments and empty lines
                if (line.empty() || line[0] == ';') {
                    continue;
                }
                
                // Parse line
                tokens = tokenize(line, 0);
                if (tokens.empty()) {
                    continue;
                }
                
                // Check for END marker
                if (tokens[0].type == TokenType::END) {
                    end_found = true;
                    break;
                }
                
                // Check for next block or directive
                if (tokens[0].type == TokenType::ADDRESS || 
                    tokens[0].type == TokenType::DIRECTIVE) {
                    // Push back the line for the next block/directive
                    input.seekg(-static_cast<int>(line.size()) - 1, std::ios_base::cur);
                    break;
                }
                
                // Parse bytes
                for (const auto& token : tokens) {
                    if (token.type == TokenType::VALUE) {
                        try {
                            uint8_t value = std::stoi(token.value, nullptr, 0);
                            block.bytes.push_back(value);
                        } catch (const std::exception& e) {
                            throw std::runtime_error("Invalid byte value: " + token.value);
                        }
                    }
                }
            }
            
            spec.code_blocks.push_back(block);
        }
    }
}

template<typename AddressT>
AddressT PhaistosParser<AddressT>::parseAddress(const Token& token) {
    std::string text = token.value;
    
    // Hexadecimal (0xNNNN, $NNNN, NNNNh)
    if ((text.size() >= 3 && text.substr(0, 2) == "0x") ||
        (text.size() >= 2 && text[0] == '$') ||
        (text.size() >= 2 && text.back() == 'h')) {
        
        std::string hexPart;
        if (text.substr(0, 2) == "0x") {
            hexPart = text.substr(2);
        } else if (text[0] == '$') {
            hexPart = text.substr(1);
        } else if (text.back() == 'h') {
            hexPart = text.substr(0, text.size() - 1);
        }
        
        return static_cast<AddressT>(std::stoi(hexPart, nullptr, 16));
    }
    
    // Binary (0bNNNNNNNN or %NNNNNNNN)
    if ((text.size() >= 3 && text.substr(0, 2) == "0b") ||
        (text.size() >= 2 && text[0] == '%')) {
        
        std::string binPart;
        if (text.substr(0, 2) == "0b") {
            binPart = text.substr(2);
        } else if (text[0] == '%') {
            binPart = text.substr(1);
        }
        
        return static_cast<AddressT>(std::stoi(binPart, nullptr, 2));
    }
    
    // Decimal
    return static_cast<AddressT>(std::stoi(text));
}

template<typename AddressT>
Value PhaistosParser<AddressT>::parseValue(const Token& token) {
    if (token.type == TokenType::ANY) {
        return Value::any();
    } else if (token.type == TokenType::SAME) {
        return Value::same();
    } else {
        return Value::parse(token.value);
    }
}

template<typename AddressT>
void PhaistosParser<AddressT>::skipCommentsAndEmptyLines(std::istream& input) {
    std::string line;
    while (input.peek() == ';' || input.peek() == '\n' || input.peek() == '\r') {
        std::getline(input, line);
    }
}

// Explicit instantiation
template class PhaistosParser<uint16_t>;

} // namespace phaistos
