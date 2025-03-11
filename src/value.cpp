/**
 * @file value.cpp
 * @brief Implementation of the Value class
 */
#include "value.hpp"
#include "logger.hpp"

namespace phaistos {

/**
 * @brief Helper function to parse numeric values from string
 * @param text String representation of a number
 * @return Parsed numeric value
 */
uint8_t parseNumeric(const std::string& text) {
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
    // Get the logger for debugging
    Logger& logger = getLogger();
    logger.debug("Parsing value from string: '" + text + "'");

    // Handle ANY variants
    if (text == "??" || text == "?" || text == "ANY" || 
        text.find("?") != std::string::npos) {
        logger.debug("  Detected ANY value");
        return any();
    }
    
    // Handle SAME keyword for preservation
    if (text == "SAME") {
        logger.debug("  Detected SAME value");
        return same();
    }
    
    // Handle EQU keyword for code equivalence
    if (text == "EQU") {
        logger.debug("  Detected EQU value");
        return equ();
    }
    
    // Parse as exact value
    try {
        uint8_t value = parseNumeric(text);
        logger.debug("  Parsed as EXACT value: 0x" + std::to_string(static_cast<int>(value)));
        return exact(value);
    } catch (const std::exception& e) {
        logger.error("  Failed to parse numeric value: " + std::string(e.what()));
        throw std::runtime_error("Failed to parse value '" + text + "': " + e.what());
    }
}

} // namespace phaistos
