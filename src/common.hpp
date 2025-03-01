/**
 * @file common.hpp
 * @brief Common includes, types, and utilities for Phaistos
 */
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace phaistos {

// Default address type for 6502 CPU
using AddressT = uint16_t;

} // namespace phaistos
