/**
 * @file value.hpp
 * @brief Value type for representing EXACT, ANY, and SAME values
 */
#pragma once

#include "common.hpp"

namespace phaistos {

/**
 * @brief Helper function to parse numeric values from string
 * @param text String representation of a number
 * @return Parsed numeric value
 */
uint8_t parseNumeric(const std::string& text);

/**
 * @class Value
 * @brief Represents a value that can be EXACT, ANY, or SAME
 */
class Value {
public:
    /**
     * @enum Type
     * @brief The type of value
     */
    enum Type { EXACT, ANY, SAME };

    /**
     * @brief Default constructor creates an ANY value
     */
    Value() : type(ANY), exact_value(0) {}

    /**
     * @brief Creates a value with the specified type and exact value
     * @param valueType The type of value
     * @param exactValue The exact value (only used if type is EXACT)
     */
    Value(Type valueType, uint8_t exactValue) : type(valueType), exact_value(exactValue) {}

    /**
     * @brief Factory method for creating an EXACT value
     * @param value The exact value
     * @return An EXACT Value
     */
    static Value exact(uint8_t value) {
        return Value(EXACT, value);
    }

    /**
     * @brief Factory method for creating an ANY value
     * @return An ANY Value
     */
    static Value any() {
        return Value(ANY, 0);
    }

    /**
     * @brief Factory method for creating a SAME value
     * @return A SAME Value
     */
    static Value same() {
        return Value(SAME, 0);
    }

    /**
     * @brief Parse a value from a string representation
     * @param text The string to parse
     * @return The parsed Value
     */
    static Value parse(const std::string& text);

    /** The type of this value */
    Type type;

    /** The exact value (only relevant if type is EXACT) */
    uint8_t exact_value;
};

} // namespace phaistos
