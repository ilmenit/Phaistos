/**
 * @file parser.cpp
 * @brief Implementation of the Phaistos parser
 */
#include "parser.hpp"
#include "value.hpp"
#include "logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace phaistos {

//----------------------------------------------------------
// Lexer static constants
//----------------------------------------------------------
template<typename AddressT>
const std::unordered_set<std::string> PhaistosParser<AddressT>::Lexer::directives = {
    "OPTIMIZE_FOR", "CPU_IN", "FLAGS_IN", "MEMORY_IN", 
    "CPU_OUT", "FLAGS_OUT", "MEMORY_OUT", "RUN",
    "OPTIMIZE", "OPTIMIZE_RO"
};

template<typename AddressT>
const std::unordered_set<std::string> PhaistosParser<AddressT>::Lexer::registers = {
    "A", "X", "Y", "SP", "PC"
};

template<typename AddressT>
const std::unordered_set<std::string> PhaistosParser<AddressT>::Lexer::flags = {
    "C", "Z", "I", "D", "B", "V", "N"
};

template<typename AddressT>
const std::unordered_set<std::string> PhaistosParser<AddressT>::Lexer::keywords = {
    "ANY", "SAME", "END", "EQU"
};

//----------------------------------------------------------
// Token methods
//----------------------------------------------------------
template<typename AddressT>
std::string PhaistosParser<AddressT>::Token::toString() const {
    std::string typeName;
    
    switch (type) {
        case TokenType::DIRECTIVE: typeName = "DIRECTIVE"; break;
        case TokenType::REGISTER: typeName = "REGISTER"; break;
        case TokenType::FLAG: typeName = "FLAG"; break;
        case TokenType::ADDRESS: typeName = "ADDRESS"; break;
        case TokenType::VALUE: typeName = "VALUE"; break;
        case TokenType::KEYWORD: typeName = "KEYWORD"; break;
        case TokenType::COLON: typeName = "COLON"; break;
        case TokenType::EQUALS: typeName = "EQUALS"; break;
        case TokenType::REPEAT: typeName = "REPEAT"; break;
        case TokenType::END_OF_LINE: typeName = "END_OF_LINE"; break;
        case TokenType::END_OF_FILE: typeName = "END_OF_FILE"; break;
        default: typeName = "UNKNOWN"; break;
    }
    
    if (type == TokenType::REPEAT) {
        return typeName + "('" + value + "', count=" + std::to_string(repeat_count) + ") at " + location.toString();
    } else {
        return typeName + "('" + value + "') at " + location.toString();
    }
}

//----------------------------------------------------------
// Lexer implementation
//----------------------------------------------------------
template<typename AddressT>
PhaistosParser<AddressT>::Lexer::Lexer(const std::string& filename)
    : filename(filename), currentLine(0), currentCol(0), tokenPeeked(false) {
    
    Logger& logger = getLogger();
    logger.debug("Opening file for parsing: " + filename);
    
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    logger.debug("Read " + std::to_string(lines.size()) + " lines from file");
    
    // Initialize with the first line if available
    if (!lines.empty()) {
        currentLineText = lines[0];
    }
}

template<typename AddressT>
bool PhaistosParser<AddressT>::Lexer::isEOF() const {
    return currentLine >= lines.size();
}

template<typename AddressT>
std::string PhaistosParser<AddressT>::Lexer::getCurrentLine() const {
    if (currentLine < lines.size()) {
        return lines[currentLine];
    }
    return "";
}

template<typename AddressT>
typename PhaistosParser<AddressT>::SourceLocation PhaistosParser<AddressT>::Lexer::getLocation() const {
    return {filename, static_cast<int>(currentLine + 1), static_cast<int>(currentCol + 1)};
}

template<typename AddressT>
void PhaistosParser<AddressT>::Lexer::skipWhitespace() {
    while (currentLine < lines.size()) {
        while (currentCol < currentLineText.size() && std::isspace(currentLineText[currentCol])) {
            currentCol++;
        }
        
        // If we found a non-whitespace character or reached a comment, stop
        if (currentCol < currentLineText.size() && currentLineText[currentCol] != ';') {
            return;
        }
        
        // Move to the next line
        nextLine();
    }
}

template<typename AddressT>
void PhaistosParser<AddressT>::Lexer::nextLine() {
    currentLine++;
    currentCol = 0;
    
    if (currentLine < lines.size()) {
        currentLineText = lines[currentLine];
    } else {
        currentLineText = "";
    }
}

template<typename AddressT>
typename PhaistosParser<AddressT>::Token PhaistosParser<AddressT>::Lexer::readIdentifier() {
    SourceLocation location = getLocation();
    std::string value;
    
    // Read the identifier
    while (currentCol < currentLineText.size() && 
           (std::isalnum(currentLineText[currentCol]) || currentLineText[currentCol] == '_')) {
        value += currentLineText[currentCol++];
    }
    
    // Determine token type
    TokenType type;
    
    if (directives.find(value) != directives.end()) {
        type = TokenType::DIRECTIVE;
    } else if (registers.find(value) != registers.end()) {
        type = TokenType::REGISTER;
    } else if (flags.find(value) != flags.end()) {
        type = TokenType::FLAG;
    } else if (keywords.find(value) != keywords.end()) {
        type = TokenType::KEYWORD;
    } else {
        type = TokenType::VALUE;
    }
    
    return Token(type, value, location);
}

template<typename AddressT>
typename PhaistosParser<AddressT>::Token PhaistosParser<AddressT>::Lexer::readNumber() {
    SourceLocation location = getLocation();
    std::string value;
    bool isAddress = false;
    
    // Check for hex prefix
    if (currentCol + 1 < currentLineText.size() && 
        currentLineText[currentCol] == '0' && 
        (currentLineText[currentCol + 1] == 'x' || currentLineText[currentCol + 1] == 'X')) {
        value += currentLineText[currentCol++]; // '0'
        value += currentLineText[currentCol++]; // 'x'
        isAddress = true;
    }
    // Check for $ prefix (alternative hex notation)
    else if (currentCol < currentLineText.size() && currentLineText[currentCol] == '$') {
        value += currentLineText[currentCol++];
        isAddress = true;
    }
    // Check for % prefix (binary)
    else if (currentCol < currentLineText.size() && currentLineText[currentCol] == '%') {
        value += currentLineText[currentCol++];
        isAddress = true;
    }
    
    // Read the remaining digits
    while (currentCol < currentLineText.size() && 
           (std::isxdigit(currentLineText[currentCol]) || 
            currentLineText[currentCol] == '?' || 
            currentLineText[currentCol] == 'h')) {  // 'h' suffix for hex
        value += currentLineText[currentCol++];
        
        // Handle the case where 'h' is used as a suffix for hex
        if (currentLineText[currentCol - 1] == 'h') {
            isAddress = true;
            break;
        }
    }
    
    // Special processing for ? (ANY) values
    if (value.find('?') != std::string::npos) {
        return Token(TokenType::KEYWORD, "ANY", location);
    }
    
    // Determine if this is an address or a regular value
    // All-hex values longer than 2 chars are likely addresses
    bool allHex = std::all_of(value.begin(), value.end(), [](char c) { 
        return std::isxdigit(c) || c == '0' || c == 'x' || c == 'X' || c == '$' || c == '%' || c == 'h';
    });
    
    if (isAddress || (allHex && value.length() > 2)) {
        return Token(TokenType::ADDRESS, value, location);
    } else {
        return Token(TokenType::VALUE, value, location);
    }
}

template<typename AddressT>
typename PhaistosParser<AddressT>::Token PhaistosParser<AddressT>::Lexer::readRepeat() {
    Logger& logger = getLogger();
    SourceLocation location = getLocation();
    
    // Consume the colon
    currentCol++;
    
    // Skip any whitespace before the digits
    while (currentCol < currentLineText.size() && std::isspace(currentLineText[currentCol])) {
        currentCol++;
    }
    
    // Read digits for the repeat count
    std::string countStr;
    while (currentCol < currentLineText.size() && std::isdigit(currentLineText[currentCol])) {
        countStr += currentLineText[currentCol++];
    }
    
    if (countStr.empty()) {
        throw std::runtime_error("Expected repeat count after ':' at " + getLocation().toString());
    }
    
    try {
        size_t count = std::stoul(countStr);
        logger.debug("Parsed repeat count: " + std::to_string(count));
        return Token(TokenType::REPEAT, ":" + countStr, location, count);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse repeat count: " + countStr + 
                               " at " + getLocation().toString() + ": " + e.what());
    }
}

template<typename AddressT>
typename PhaistosParser<AddressT>::Token PhaistosParser<AddressT>::Lexer::nextToken() {
    Logger& logger = getLogger();
    
    // If we've already peeked at a token, return it
    if (tokenPeeked) {
        tokenPeeked = false;
        return currentToken;
    }
    
    // Skip any whitespace
    skipWhitespace();
    
    // Check for end of file
    if (currentLine >= lines.size()) {
        return Token(TokenType::END_OF_FILE, "", getLocation());
    }
    
    // Check for end of line
    if (currentCol >= currentLineText.size() || currentLineText[currentCol] == ';') {
        SourceLocation loc = getLocation();
        nextLine();
        return Token(TokenType::END_OF_LINE, "", loc);
    }
    
    // Process the current character
    char c = currentLineText[currentCol];
    
    // Check for special characters
    if (c == ':') {
        // Check if this is a repeat count (e.g., ":4")
        size_t nextCol = currentCol + 1;
        // Skip any whitespace
        while (nextCol < currentLineText.size() && std::isspace(currentLineText[nextCol])) {
            nextCol++;
        }
        
        // If followed by digits, it's a repeat count
        if (nextCol < currentLineText.size() && std::isdigit(currentLineText[nextCol])) {
            return readRepeat();
        } else {
            // It's just a regular colon
            currentCol++;
            return Token(TokenType::COLON, ":", getLocation());
        }
    } else if (c == '=') {
        currentCol++;
        return Token(TokenType::EQUALS, "=", getLocation());
    }
    // Check for numbers and addresses
    else if (std::isdigit(c) || c == '$' || c == '%') {
        return readNumber();
    }
    // Otherwise it's an identifier
    else if (std::isalpha(c) || c == '_') {
        return readIdentifier();
    }
    // Unknown character
    else {
        logger.warning("Unknown character in input: '" + std::string(1, c) + 
                      "' at " + getLocation().toString());
        currentCol++; // Skip the unknown character
        return nextToken(); // Get the next valid token
    }
}

template<typename AddressT>
typename PhaistosParser<AddressT>::Token PhaistosParser<AddressT>::Lexer::peekToken() {
    if (!tokenPeeked) {
        currentToken = nextToken();
        tokenPeeked = true;
    }
    return currentToken;
}

//----------------------------------------------------------
// Parser implementation
//----------------------------------------------------------
template<typename AddressT>
OptimizationSpec<AddressT> PhaistosParser<AddressT>::parse(const std::string& filename) {
    Logger& logger = getLogger();
    logger.info("Parsing specification file: " + filename);
    
    try {
        Lexer lexer(filename);
        OptimizationSpec<AddressT> spec = parseSpecification(lexer);
        logger.info("Successfully parsed specification file");
        return spec;
    }
    catch (const std::exception& e) {
        logger.error("Error parsing specification: " + std::string(e.what()));
        throw;
    }
}

template<typename AddressT>
OptimizationSpec<AddressT> PhaistosParser<AddressT>::parseSpecification(Lexer& lexer) {
    Logger& logger = getLogger();
    OptimizationSpec<AddressT> spec;
    
    // Process each directive in the file
    while (!lexer.isEOF()) {
        Token token = lexer.nextToken();
        
        // Skip end of line tokens
        if (token.type == TokenType::END_OF_LINE) {
            continue;
        }
        
        // Check for end of file
        if (token.type == TokenType::END_OF_FILE) {
            break;
        }
        
        // Process directives
        if (token.type == TokenType::DIRECTIVE) {
            std::string directive = token.value;
            logger.debug("Processing directive: " + directive);
            
            if (directive == "OPTIMIZE_FOR") {
                parseOptimizationGoal(lexer, spec);
            }
            else if (directive == "CPU_IN") {
                parseCPUState(lexer, spec.input_cpu, false);
            }
            else if (directive == "FLAGS_IN") {
                parseFlagState(lexer, spec.input_flags, false);
            }
            else if (directive == "MEMORY_IN") {
                parseMemoryRegions(lexer, spec.input_memory, false);
            }
            else if (directive == "CPU_OUT") {
                parseCPUState(lexer, spec.output_cpu, true);
            }
            else if (directive == "FLAGS_OUT") {
                parseFlagState(lexer, spec.output_flags, true);
            }
            else if (directive == "MEMORY_OUT") {
                parseMemoryRegions(lexer, spec.output_memory, true);
            }
            else if (directive == "OPTIMIZE") {
                parseOptimizeBlock(lexer, spec, false);
            }
            else if (directive == "OPTIMIZE_RO") {
                parseOptimizeBlock(lexer, spec, true);
            }
            else if (directive == "RUN") {
                parseRunAddress(lexer, spec);
            }
            else {
                throw std::runtime_error(formatError("Unknown directive", token));
            }
        }
        else {
            throw std::runtime_error(formatError("Expected directive, got", token));
        }
    }
    
    // Validate the specification
    if (spec.run_address == 0) {
        logger.warning("No RUN address specified in the specification");
    }
    
    return spec;
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseOptimizationGoal(Lexer& lexer, OptimizationSpec<AddressT>& spec) {
    Logger& logger = getLogger();
    
    // Expect a colon
    Token token = lexer.nextToken();
    if (token.type != TokenType::COLON) {
        throw std::runtime_error(formatError("Expected ':', got", token));
    }
    
    // Get the goal
    token = lexer.nextToken();
    if (token.type != TokenType::VALUE) {
        throw std::runtime_error(formatError("Expected goal (size or speed), got", token));
    }
    
    std::string goal = token.value;
    std::transform(goal.begin(), goal.end(), goal.begin(), ::tolower);
    
    if (goal == "size") {
        spec.goal = OptimizationSpec<AddressT>::SIZE;
        logger.debug("Setting optimization goal to SIZE");
    }
    else if (goal == "speed") {
        spec.goal = OptimizationSpec<AddressT>::SPEED;
        logger.debug("Setting optimization goal to SPEED");
    }
    else {
        throw std::runtime_error(formatError("Invalid optimization goal, expected 'size' or 'speed', got", token));
    }
    
    // Skip to the end of the line
    while (token.type != TokenType::END_OF_LINE && token.type != TokenType::END_OF_FILE) {
        token = lexer.nextToken();
    }
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseCPUState(Lexer& lexer, 
                                          typename OptimizationSpec<AddressT>::CPUState& state, 
                                          bool isOutput) {
    Logger& logger = getLogger();
    logger.debug("Parsing CPU state (isOutput=" + std::string(isOutput ? "true" : "false") + ")");
    
    // Process each register assignment
    Token token = lexer.nextToken();
    while (token.type != TokenType::DIRECTIVE && token.type != TokenType::END_OF_FILE) {
        // Skip end of line tokens
        if (token.type == TokenType::END_OF_LINE) {
            token = lexer.nextToken();
            continue;
        }
        
        // Expect a register
        if (token.type != TokenType::REGISTER) {
            throw std::runtime_error(formatError("Expected register name, got", token));
        }
        
        std::string reg = token.value;
        
        // Expect a colon or equals sign
        token = lexer.nextToken();
        if (token.type != TokenType::COLON && token.type != TokenType::EQUALS) {
            throw std::runtime_error(formatError("Expected ':' or '=' after register name, got", token));
        }
        
        // Get the value
        token = lexer.nextToken();
        Value value;
        
        try {
            // Parse the value with context awareness
            value = parseValue(token, isOutput);
            logger.debug("Register " + reg + " value: " + token.value + 
                        " (type=" + std::to_string(value.type) + ")");
        } catch (const std::exception& e) {
            throw std::runtime_error(formatError(e.what(), token));
        }
        
        // Set the register value
        if (reg == "A") {
            state.a = value;
        }
        else if (reg == "X") {
            state.x = value;
        }
        else if (reg == "Y") {
            state.y = value;
        }
        else if (reg == "SP") {
            state.sp = value;
        }
        else {
            throw std::runtime_error(formatError("Unknown register", token));
        }
        
        // Skip to the end of the line
        while (token.type != TokenType::END_OF_LINE && token.type != TokenType::END_OF_FILE) {
            token = lexer.nextToken();
        }
        
        // Get the next token
        token = lexer.nextToken();
    }
    
    // Push back the directive token
    if (token.type == TokenType::DIRECTIVE) {
        lexer.peekToken(); // This will make the next call to nextToken() return the same token
    }
    
    logger.debug("Finished parsing CPU state");
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseFlagState(Lexer& lexer, 
                                           typename OptimizationSpec<AddressT>::FlagState& flags, 
                                           bool isOutput) {
    Logger& logger = getLogger();
    logger.debug("Parsing flag state (isOutput=" + std::string(isOutput ? "true" : "false") + ")");
    
    // Process each flag assignment
    Token token = lexer.nextToken();
    while (token.type != TokenType::DIRECTIVE && token.type != TokenType::END_OF_FILE) {
        // Skip end of line tokens
        if (token.type == TokenType::END_OF_LINE) {
            token = lexer.nextToken();
            continue;
        }
        
        // Expect a flag
        if (token.type != TokenType::FLAG) {
            throw std::runtime_error(formatError("Expected flag name, got", token));
        }
        
        std::string flag = token.value;
        
        // Expect a colon or equals sign
        token = lexer.nextToken();
        if (token.type != TokenType::COLON && token.type != TokenType::EQUALS) {
            throw std::runtime_error(formatError("Expected ':' or '=' after flag name, got", token));
        }
        
        // Get the value
        token = lexer.nextToken();
        Value value;
        
        try {
            // Parse the value with context awareness
            value = parseValue(token, isOutput);
            logger.debug("Flag " + flag + " value: " + token.value);
        } catch (const std::exception& e) {
            throw std::runtime_error(formatError(e.what(), token));
        }
        
        // Set the flag value
        if (flag == "C") {
            flags.c = value;
        }
        else if (flag == "Z") {
            flags.z = value;
        }
        else if (flag == "I") {
            flags.i = value;
        }
        else if (flag == "D") {
            flags.d = value;
        }
        else if (flag == "B") {
            flags.b = value;
        }
        else if (flag == "V") {
            flags.v = value;
        }
        else if (flag == "N") {
            flags.n = value;
        }
        else {
            throw std::runtime_error(formatError("Unknown flag", token));
        }
        
        // Skip to the end of the line
        while (token.type != TokenType::END_OF_LINE && token.type != TokenType::END_OF_FILE) {
            token = lexer.nextToken();
        }
        
        // Get the next token
        token = lexer.nextToken();
    }
    
    // Push back the directive token
    if (token.type == TokenType::DIRECTIVE) {
        lexer.peekToken(); // This will make the next call to nextToken() return the same token
    }
    
    logger.debug("Finished parsing flag state");
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseMemoryValues(Lexer& lexer,
                                             typename OptimizationSpec<AddressT>::MemoryRegion& region,
                                             bool isOutput) {
    Logger& logger = getLogger();
    Token token = lexer.nextToken();
    
    // Process values until end of line or EOF
    while (token.type != TokenType::END_OF_LINE && token.type != TokenType::END_OF_FILE) {
        if (token.type == TokenType::REPEAT) {
            // Handle a repeated value
            size_t repeatCount = token.repeat_count;
            
            // Get the value to repeat
            token = lexer.nextToken();
            
            // Check for EOF
            if (token.type == TokenType::END_OF_FILE) {
                throw std::runtime_error("Unexpected end of file after repeat count");
            }
            
            // Parse the value
            Value repeatedValue;
            try {
                repeatedValue = parseValue(token, isOutput);
                logger.debug("Parsing repeated value: " + token.value + " x " + std::to_string(repeatCount));
            } catch (const std::exception& e) {
                throw std::runtime_error(formatError(e.what(), token));
            }
            
            // Add the specified number of copies of the value
            for (size_t i = 0; i < repeatCount; ++i) {
                region.bytes.push_back(repeatedValue);
            }
        } else {
            // Parse regular value
            try {
                Value value = parseValue(token, isOutput);
                region.bytes.push_back(value);
                logger.debug("Added byte value: " + token.value);
            } catch (const std::exception& e) {
                throw std::runtime_error(formatError(e.what(), token));
            }
        }
        
        // Get the next token
        token = lexer.nextToken();
    }
}

template<typename AddressT>
typename OptimizationSpec<AddressT>::MemoryRegion PhaistosParser<AddressT>::parseMemoryRegion(Lexer& lexer, bool isOutput) {
    Logger& logger = getLogger();
    
    // Expect an address
    Token token = lexer.nextToken();
    if (token.type != TokenType::ADDRESS) {
        throw std::runtime_error(formatError("Expected address, got", token));
    }
    
    // Parse the address
    AddressT address = parseAddress(token);
    
    // Expect a colon
    token = lexer.nextToken();
    if (token.type != TokenType::COLON) {
        throw std::runtime_error(formatError("Expected ':' after address, got", token));
    }
    
    // Create a new memory region
    typename OptimizationSpec<AddressT>::MemoryRegion region;
    region.address = address;
    
    // Parse the memory values
    parseMemoryValues(lexer, region, isOutput);
    
    // Return the populated region
    return region;
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseMemoryRegions(Lexer& lexer, 
                                             std::vector<typename OptimizationSpec<AddressT>::MemoryRegion>& regions,
                                             bool isOutput) {
    Logger& logger = getLogger();
    logger.debug("Parsing memory regions (isOutput=" + std::string(isOutput ? "true" : "false") + ")");
    
    // Process each memory region
    Token token = lexer.nextToken();
    while (token.type != TokenType::DIRECTIVE && token.type != TokenType::END_OF_FILE) {
        // Skip end of line tokens
        if (token.type == TokenType::END_OF_LINE) {
            token = lexer.nextToken();
            continue;
        }
        
        // Put the token back so parseMemoryRegion can process it
        lexer.peekToken();
        
        // Parse the memory region
        typename OptimizationSpec<AddressT>::MemoryRegion region = parseMemoryRegion(lexer, isOutput);
        
        // Add the region to the list if it's not empty
        if (!region.bytes.empty()) {
            regions.push_back(region);
            logger.debug("Added memory region at address 0x" + 
                       std::to_string(region.address) + 
                       " with " + std::to_string(region.bytes.size()) + " bytes");
        }
        
        // Get the next token
        token = lexer.nextToken();
    }
    
    // Push back the directive token
    if (token.type == TokenType::DIRECTIVE) {
        lexer.peekToken();
    }
    
    logger.debug("Finished parsing memory regions, found " + std::to_string(regions.size()) + " regions");
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseOptimizeBlock(Lexer& lexer, OptimizationSpec<AddressT>& spec, bool readOnly) {
    Logger& logger = getLogger();
    logger.debug("Parsing " + std::string(readOnly ? "OPTIMIZE_RO" : "OPTIMIZE") + " block");
    
    // Process each code block
    Token token = lexer.nextToken();
    while (token.type != TokenType::DIRECTIVE && token.type != TokenType::END_OF_FILE) {
        // Skip end of line tokens
        if (token.type == TokenType::END_OF_LINE) {
            token = lexer.nextToken();
            continue;
        }
        
        // Expect an address
        if (token.type != TokenType::ADDRESS) {
            throw std::runtime_error(formatError("Expected address, got", token));
        }
        
        // Parse the address
        AddressT address = parseAddress(token);
        
        // Expect a colon
        token = lexer.nextToken();
        if (token.type != TokenType::COLON) {
            throw std::runtime_error(formatError("Expected ':' after address, got", token));
        }
        
        // Create a new code block
        typename OptimizationSpec<AddressT>::CodeBlock block;
        block.address = address;
        block.type = readOnly ? 
                    OptimizationSpec<AddressT>::CodeBlock::READ_ONLY : 
                    OptimizationSpec<AddressT>::CodeBlock::REGULAR;
        
        // Check for empty block (END marker on the same line)
        token = lexer.nextToken();
        if (token.type == TokenType::KEYWORD && token.value == "END") {
            // Empty block for code synthesis
            logger.debug("Found empty code block for synthesis at address 0x" + std::to_string(address));
            spec.code_blocks.push_back(block);
            
            // Skip to the end of the line
            while (token.type != TokenType::END_OF_LINE && token.type != TokenType::END_OF_FILE) {
                token = lexer.nextToken();
            }
            
            // Get the next token
            token = lexer.nextToken();
            continue;
        }
        
        // Parse the block contents
        bool endFound = false;
        
        // First handle values on the same line
        while (token.type != TokenType::END_OF_LINE && token.type != TokenType::END_OF_FILE) {
            if (token.type == TokenType::KEYWORD && token.value == "END") {
                endFound = true;
                break;
            }
            
            // Skip ANY values
            if (token.type == TokenType::KEYWORD && token.value == "ANY") {
                logger.debug("Skipping ANY value in code block");
            }
            else if (token.type == TokenType::VALUE || token.type == TokenType::ADDRESS) {
                try {
                    uint8_t byte = parseByte(token);
                    block.bytes.push_back(byte);
                    logger.debug("Added byte 0x" + std::to_string(static_cast<int>(byte)) + " to code block");
                }
                catch (const std::exception& e) {
                    logger.warning("Failed to parse byte value: " + token.value + 
                                 " at " + token.location.toString() + ": " + e.what());
                }
            }
            
            token = lexer.nextToken();
        }
        
        // Then parse values on subsequent lines
        if (!endFound) {
            token = lexer.nextToken();
            
            while (token.type != TokenType::DIRECTIVE && 
                   token.type != TokenType::END_OF_FILE && 
                   !(token.type == TokenType::ADDRESS && lexer.peekToken().type == TokenType::COLON)) {
                
                // Skip end of line tokens
                if (token.type == TokenType::END_OF_LINE) {
                    token = lexer.nextToken();
                    continue;
                }
                
                // Check for END marker
                if (token.type == TokenType::KEYWORD && token.value == "END") {
                    endFound = true;
                    
                    // Skip to the end of the line
                    while (token.type != TokenType::END_OF_LINE && token.type != TokenType::END_OF_FILE) {
                        token = lexer.nextToken();
                    }
                    
                    break;
                }
                
                // Skip ANY values
                if (token.type == TokenType::KEYWORD && token.value == "ANY") {
                    logger.debug("Skipping ANY value in code block");
                }
                else if (token.type == TokenType::VALUE || token.type == TokenType::ADDRESS) {
                    try {
                        uint8_t byte = parseByte(token);
                        block.bytes.push_back(byte);
                        logger.debug("Added byte 0x" + std::to_string(static_cast<int>(byte)) + " to code block");
                    }
                    catch (const std::exception& e) {
                        logger.warning("Failed to parse byte value: " + token.value + 
                                     " at " + token.location.toString() + ": " + e.what());
                    }
                }
                
                // Skip to the end of the line
                while (token.type != TokenType::END_OF_LINE && token.type != TokenType::END_OF_FILE) {
                    token = lexer.nextToken();
                }
                
                // Get the next token
                token = lexer.nextToken();
            }
        }
        
        // Add the block to the list
        spec.code_blocks.push_back(block);
        logger.debug("Added code block at address 0x" + 
                   std::to_string(block.address) + 
                   " with " + std::to_string(block.bytes.size()) + " bytes");
        
        // Continue with the next block
        if (token.type == TokenType::ADDRESS) {
            // Keep the token for the next block
            lexer.peekToken();
        }
        else if (token.type == TokenType::DIRECTIVE) {
            // Keep the directive token
            lexer.peekToken();
            break;
        }
    }
    
    logger.debug("Finished parsing " + std::string(readOnly ? "OPTIMIZE_RO" : "OPTIMIZE") + 
               " block, total blocks: " + std::to_string(spec.code_blocks.size()));
}

template<typename AddressT>
void PhaistosParser<AddressT>::parseRunAddress(Lexer& lexer, OptimizationSpec<AddressT>& spec) {
    Logger& logger = getLogger();
    
    // Expect a colon
    Token token = lexer.nextToken();
    if (token.type != TokenType::COLON) {
        throw std::runtime_error(formatError("Expected ':' after RUN, got", token));
    }
    
    // Get the address
    token = lexer.nextToken();
    if (token.type != TokenType::ADDRESS && token.type != TokenType::VALUE) {
        throw std::runtime_error(formatError("Expected address after RUN:, got", token));
    }
    
    // Parse the address
    spec.run_address = parseAddress(token);
    logger.debug("Set run address to 0x" + std::to_string(spec.run_address));
    
    // Skip to the end of the line
    while (token.type != TokenType::END_OF_LINE && token.type != TokenType::END_OF_FILE) {
        token = lexer.nextToken();
    }
}

template<typename AddressT>
AddressT PhaistosParser<AddressT>::parseAddress(const Token& token) {
    Logger& logger = getLogger();
    std::string text = token.value;
    
    try {
        // Hexadecimal (0xNNNN)
        if (text.find("0x") == 0 || text.find("0X") == 0) {
            AddressT result = static_cast<AddressT>(std::stoi(text.substr(2), nullptr, 16));
            logger.debug("Parsed hexadecimal address: 0x" + std::to_string(result));
            return result;
        }
        // Hexadecimal ($NNNN)
        else if (text.find("$") == 0) {
            AddressT result = static_cast<AddressT>(std::stoi(text.substr(1), nullptr, 16));
            logger.debug("Parsed hexadecimal address: 0x" + std::to_string(result));
            return result;
        }
        // Hexadecimal (NNNNh)
        else if (text.size() >= 2 && text.back() == 'h') {
            AddressT result = static_cast<AddressT>(std::stoi(text.substr(0, text.size() - 1), nullptr, 16));
            logger.debug("Parsed hexadecimal address: 0x" + std::to_string(result));
            return result;
        }
        // Binary (0bNNNNNNNN)
        else if (text.find("0b") == 0 || text.find("0B") == 0) {
            AddressT result = static_cast<AddressT>(std::stoi(text.substr(2), nullptr, 2));
            logger.debug("Parsed binary address: 0x" + std::to_string(result));
            return result;
        }
        // Binary (%NNNNNNNN)
        else if (text.find("%") == 0) {
            AddressT result = static_cast<AddressT>(std::stoi(text.substr(1), nullptr, 2));
            logger.debug("Parsed binary address: 0x" + std::to_string(result));
            return result;
        }
        // Try as hex if all hex digits
        else if (std::all_of(text.begin(), text.end(), [](char c) { 
            return std::isxdigit(c); 
        })) {
            AddressT result = static_cast<AddressT>(std::stoi(text, nullptr, 16));
            logger.debug("Parsed implicit hexadecimal address: 0x" + std::to_string(result));
            return result;
        }
        // Decimal
        else {
            AddressT result = static_cast<AddressT>(std::stoi(text));
            logger.debug("Parsed decimal address: 0x" + std::to_string(result));
            return result;
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error(formatError("Failed to parse address: " + std::string(e.what()), token));
    }
}

template<typename AddressT>
Value PhaistosParser<AddressT>::parseValue(const Token& token, bool isOutput) {
    Logger& logger = getLogger();
    
    try {
        // Check for special values
        if (token.type == TokenType::KEYWORD) {
            if (token.value == "ANY") {
                logger.debug("Parsed ANY value");
                return Value::any();
            }
            else if (token.value == "SAME") {
                // SAME is only valid in output contexts
                if (!isOutput) {
                    throw std::runtime_error("SAME value is only valid in output contexts");
                }
                logger.debug("Parsed SAME value");
                return Value::same();
            }
            else if (token.value == "EQU") {
                // EQU is only valid in output contexts
                if (!isOutput) {
                    throw std::runtime_error("EQU value is only valid in output contexts");
                }
                logger.debug("Parsed EQU value");
                return Value::equ();
            }
        }
        
        // Otherwise parse as a regular value (handles numeric values and ANY variants with ? marks)
        Value result = Value::parse(token.value);
        
        if (result.type == Value::EXACT) {
            logger.debug("Parsed EXACT value: 0x" + std::to_string(static_cast<int>(result.exact_value)));
        }
        else {
            logger.debug("Parsed non-EXACT value type: " + std::to_string(result.type));
        }
        
        return result;
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse value: " + std::string(e.what()));
    }
}

template<typename AddressT>
uint8_t PhaistosParser<AddressT>::parseByte(const Token& token) {
    Logger& logger = getLogger();
    std::string text = token.value;
    
    try {
        // If the value contains a '?', it's an ANY value, which we skip
        if (text.find('?') != std::string::npos) {
            throw std::runtime_error("ANY values are not allowed in code blocks");
        }
        
        int value = 0;
        
        // Hexadecimal (0xNN)
        if (text.find("0x") == 0 || text.find("0X") == 0) {
            value = std::stoi(text.substr(2), nullptr, 16);
        }
        // Hexadecimal ($NN)
        else if (text.find("$") == 0) {
            value = std::stoi(text.substr(1), nullptr, 16);
        }
        // Hexadecimal (NNh)
        else if (text.size() >= 2 && text.back() == 'h') {
            value = std::stoi(text.substr(0, text.size() - 1), nullptr, 16);
        }
        // Binary (0bNNNNNNNN)
        else if (text.find("0b") == 0 || text.find("0B") == 0) {
            value = std::stoi(text.substr(2), nullptr, 2);
        }
        // Binary (%NNNNNNNN)
        else if (text.find("%") == 0) {
            value = std::stoi(text.substr(1), nullptr, 2);
        }
        // Try as hex if all hex digits
        else if (std::all_of(text.begin(), text.end(), [](char c) { 
            return std::isxdigit(c); 
        })) {
            value = std::stoi(text, nullptr, 16);
        }
        // Decimal
        else {
            value = std::stoi(text);
        }
        
        // Ensure the value fits in a byte
        if (value < 0 || value > 255) {
            logger.warning("Value " + std::to_string(value) + " truncated to fit in byte");
        }
        
        return static_cast<uint8_t>(value & 0xFF);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(formatError("Failed to parse byte value: " + std::string(e.what()), token));
    }
}

template<typename AddressT>
std::string PhaistosParser<AddressT>::formatError(const std::string& message, const Token& token) {
    std::ostringstream oss;
    oss << message << " " << token.toString() << std::endl;
    oss << "Line: " << token.location.line << ": " << Lexer(token.location.filename).getCurrentLine();
    return oss.str();
}

// Explicit instantiation for uint16_t
template class PhaistosParser<uint16_t>;

} // namespace phaistos