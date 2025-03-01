/**
 * @file value.cpp
 * @brief Implementation of the Value class
 */
#include "value.hpp"

namespace phaistos {

/**
 * @brief Helper function to parse numeric values from string
 * @param text String representation of a number
 * @return Parsed numeric value
 */
static uint8_t parseNumeric(const std::string& text) {
    if (text.empty()) {
        throw std::runtime_error("Empty value string");
    }

    // Hexadecimal (0xNN, $NN, NNh)
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
        
        return static_cast<uint8_t>(std::stoi(hexPart, nullptr, 16));
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
        
        return static_cast<uint8_t>(std::stoi(binPart, nullptr, 2));
    }
    
    // Decimal
    return static_cast<uint8_t>(std::stoi(text));
}

Value Value::parse(const std::string& text) {
    // Handle ANY variants
    if (text == "??" || text == "?" || text == "ANY") {
        return any();
    }
    
    // Handle SAME keyword for preservation
    if (text == "SAME") {
        return same();
    }
    
    // Handle 0x? or $? notation
    if ((text.size() >= 3 && text.substr(0, 2) == "0x" && 
         text.substr(2) == "?") ||
        (text.size() >= 2 && text[0] == '$' && 
         text.substr(1) == "?")) {
        return any();
    }
    
    // Parse as exact value
    uint8_t value = parseNumeric(text);
    return exact(value);
}

} // namespace phaistos
