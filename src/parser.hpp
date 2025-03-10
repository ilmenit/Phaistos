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
     *
     * This parser implements a two-phase parsing process:
     * 1. Lexical analysis (tokenization)
     * 2. Syntactic analysis (parsing)
     *
     * It maintains precise location information for error reporting
     * and implements a clean separation of concerns.
     */
    template<typename AddressT = uint16_t>
    class PhaistosParser {
    public:
        /**
         * @brief Parse a .pha file into an OptimizationSpec
         * @param filename Path to the file
         * @return The parsed OptimizationSpec
         * @throws std::runtime_error on parsing errors with detailed location info
         */
        OptimizationSpec<AddressT> parse(const std::string& filename);

    private:
        /**
         * @enum TokenType
         * @brief Types of tokens in the .pha format
         */
        enum class TokenType {
            DIRECTIVE,      // OPTIMIZE_FOR, CPU_IN, etc.
            REGISTER,       // A, X, Y, SP
            FLAG,           // C, Z, I, D, B, V, N
            ADDRESS,        // 0x1000, $FF, etc.
            VALUE,          // Regular value
            KEYWORD,        // ANY, SAME, END
            COLON,          // :
            EQUALS,         // =
            END_OF_LINE,    // End of a line
            END_OF_FILE     // End of the file
        };

        /**
         * @struct SourceLocation
         * @brief Stores source code location information
         */
        struct SourceLocation {
            std::string filename;
            int line;
            int column;

            // Default constructor
            SourceLocation() : filename(""), line(0), column(0) {}

            // Parameterized constructor
            SourceLocation(const std::string& fname, int l, int c)
                : filename(fname), line(l), column(c) {
            }

            std::string toString() const {
                return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
            }
        };

        /**
         * @struct Token
         * @brief A token from the lexical analysis phase
         */
        struct Token {
            TokenType type;
            std::string value;
            SourceLocation location;

            // Default constructor
            Token() : type(TokenType::END_OF_FILE), value(""), location() {}

            // Parameterized constructor
            Token(TokenType t, const std::string& v, const SourceLocation& loc)
                : type(t), value(v), location(loc) {
            }

            std::string toString() const;
        };

        /**
         * @class Lexer
         * @brief Lexical analyzer for Phaistos files
         */
        class Lexer {
        public:
            /**
             * @brief Constructor
             * @param filename The file to tokenize
             */
            explicit Lexer(const std::string& filename);

            /**
             * @brief Get the next token
             * @return The next token
             */
            Token nextToken();

            /**
             * @brief Peek at the next token without consuming it
             * @return The next token
             */
            Token peekToken();

            /**
             * @brief Check if we've reached the end of the file
             * @return True if at end of file
             */
            bool isEOF() const;

            /**
             * @brief Get the current line text
             * @return Current line text
             */
            std::string getCurrentLine() const;

        private:
            std::vector<std::string> lines;
            std::string filename;
            size_t currentLine;
            size_t currentCol;
            std::string currentLineText;
            Token currentToken;
            bool tokenPeeked;

            // Set of known directives
            static const std::unordered_set<std::string> directives;

            // Set of known registers
            static const std::unordered_set<std::string> registers;

            // Set of known flags
            static const std::unordered_set<std::string> flags;

            // Set of known keywords
            static const std::unordered_set<std::string> keywords;

            /**
             * @brief Skip whitespace
             */
            void skipWhitespace();

            /**
             * @brief Skip to the next line
             */
            void nextLine();

            /**
             * @brief Get the current source location
             * @return Current location
             */
            SourceLocation getLocation() const;

            /**
             * @brief Read an identifier
             * @return The identifier token
             */
            Token readIdentifier();

            /**
             * @brief Read a number or address
             * @return The number/address token
             */
            Token readNumber();
        };

        /**
         * @brief Parse a complete specification
         * @param lexer The lexer to use
         * @return The parsed specification
         */
        OptimizationSpec<AddressT> parseSpecification(Lexer& lexer);

        /**
         * @brief Parse an optimization goal directive
         * @param lexer The lexer to use
         * @param spec The specification to update
         */
        void parseOptimizationGoal(Lexer& lexer, OptimizationSpec<AddressT>& spec);

        /**
         * @brief Parse CPU state (registers)
         * @param lexer The lexer to use
         * @param state The CPU state to update
         */
        void parseCPUState(Lexer& lexer, typename OptimizationSpec<AddressT>::CPUState& state);

        /**
         * @brief Parse flag state
         * @param lexer The lexer to use
         * @param flags The flag state to update
         */
        void parseFlagState(Lexer& lexer, typename OptimizationSpec<AddressT>::FlagState& flags);

        /**
         * @brief Parse memory regions
         * @param lexer The lexer to use
         * @param regions The vector of memory regions to update
         */
        void parseMemoryRegions(Lexer& lexer, std::vector<typename OptimizationSpec<AddressT>::MemoryRegion>& regions);

        /**
         * @brief Parse a memory region
         * @param lexer The lexer to use
         * @return The parsed memory region
         */
        typename OptimizationSpec<AddressT>::MemoryRegion parseMemoryRegion(Lexer& lexer);

        /**
         * @brief Parse an optimize block
         * @param lexer The lexer to use
         * @param spec The specification to update
         * @param readOnly Whether this is a read-only block
         */
        void parseOptimizeBlock(Lexer& lexer, OptimizationSpec<AddressT>& spec, bool readOnly);

        /**
         * @brief Parse a code block
         * @param lexer The lexer to use
         * @param type The type of code block
         * @return The parsed code block
         */
        typename OptimizationSpec<AddressT>::CodeBlock parseCodeBlock(Lexer& lexer,
            typename OptimizationSpec<AddressT>::CodeBlock::Type type);

        /**
         * @brief Parse a run address
         * @param lexer The lexer to use
         * @param spec The specification to update
         */
        void parseRunAddress(Lexer& lexer, OptimizationSpec<AddressT>& spec);

        /**
         * @brief Parse an address
         * @param token The token containing the address
         * @return The parsed address
         */
        AddressT parseAddress(const Token& token);

        /**
         * @brief Parse a value
         * @param token The token containing the value
         * @return The parsed value
         */
        Value parseValue(const Token& token);

        /**
         * @brief Parse a byte value
         * @param token The token containing the byte value
         * @return The parsed byte value
         */
        uint8_t parseByte(const Token& token);

        /**
         * @brief Format an error message with source location
         * @param message The base error message
         * @param token The token where the error occurred
         * @return Formatted error message
         */
        std::string formatError(const std::string& message, const Token& token);
    };

} // namespace phaistos