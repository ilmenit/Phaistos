/**
 * @file parser.hpp
 * @brief Parser for Phaistos specification files
 */
#pragma once

#include "common.hpp"
#include "optimization_spec.hpp"

namespace phaistos {

/**
 * @class PhaistosParser
 * @brief Parses .pha files into OptimizationSpec objects
 */
template<typename AddressT = uint16_t>
class PhaistosParser {
public:
    /**
     * @brief Parse a .pha file into an OptimizationSpec
     * @param filename Path to the file
     * @return The parsed OptimizationSpec
     * @throws std::runtime_error on parsing errors
     */
    OptimizationSpec<AddressT> parse(const std::string& filename);

private:
    /**
     * @enum TokenType
     * @brief Types of tokens in the .pha format
     */
    enum class TokenType {
        DIRECTIVE, REGISTER, FLAG, ADDRESS, VALUE, ANY, SAME,
        EQUALS, COLON, SEMICOLON, NEWLINE, END, EOF_TOKEN
    };
    
    /**
     * @struct Token
     * @brief Token from the lexical analysis
     */
    struct Token {
        TokenType type;
        std::string value;
        int line;
        int column;
        
        Token(TokenType t, const std::string& v, int l, int c)
            : type(t), value(v), line(l), column(c) {}
    };
    
    /**
     * @brief Tokenize a line of text
     * @param line Line to tokenize
     * @return Vector of tokens
     */
    std::vector<Token> tokenize(const std::string& line, int line_number);
    
    /**
     * @brief Parse an optimization goal directive
     * @param tokens Tokens to parse
     * @param spec Spec to update
     */
    void parseOptimizationGoal(const std::vector<Token>& tokens, 
                              OptimizationSpec<AddressT>& spec);
    
    /**
     * @brief Parse CPU state
     * @param input Input stream
     * @param state CPU state to update
     */
    void parseCPUState(std::istream& input, 
                      typename OptimizationSpec<AddressT>::CPUState& state);
    
    /**
     * @brief Parse flag state
     * @param input Input stream
     * @param flags Flag state to update
     */
    void parseFlagState(std::istream& input, 
                       typename OptimizationSpec<AddressT>::FlagState& flags);
    
    /**
     * @brief Parse memory regions
     * @param input Input stream
     * @param regions Memory regions to update
     */
    void parseMemoryRegions(std::istream& input, 
                           std::vector<typename OptimizationSpec<AddressT>::MemoryRegion>& regions);
    
    /**
     * @brief Parse memory region in horizontal format
     * @param tokens Tokens to parse
     * @return Parsed memory region
     */
    typename OptimizationSpec<AddressT>::MemoryRegion parseHorizontalMemoryRegion(
        const std::vector<Token>& tokens);
    
    /**
     * @brief Parse run address
     * @param tokens Tokens to parse
     * @param spec Spec to update
     */
    void parseRunAddress(const std::vector<Token>& tokens, 
                        OptimizationSpec<AddressT>& spec);
    
    /**
     * @brief Parse OPTIMIZE or OPTIMIZE_RO blocks
     * @param input Input stream
     * @param spec Spec to update
     * @param read_only Whether this is an OPTIMIZE_RO block
     */
    void parseOptimizeBlock(std::istream& input, 
                           OptimizationSpec<AddressT>& spec,
                           bool read_only);
    
    /**
     * @brief Parse an address token
     * @param token Token to parse
     * @return Parsed address
     */
    AddressT parseAddress(const Token& token);
    
    /**
     * @brief Parse a value token
     * @param token Token to parse
     * @return Parsed value
     */
    Value parseValue(const Token& token);
    
    /**
     * @brief Skip comments and empty lines
     * @param input Input stream
     */
    void skipCommentsAndEmptyLines(std::istream& input);
};

} // namespace phaistos
