/**
 * @file LZSS.hpp
 * @date 2026-04-28
 * @brief Declares LZSS compression and decompression helpers.
 * @author Dominik Honza xhonza04
 */

#pragma once

#include "ConfigProvider.hpp"
#include <cstdint>
#include <vector>

/**
 * Stateless LZSS codec used for individual serialized image blocks.
 */
class LZSS {
public:
    /**
     * Compresses one byte stream using LZSS.
     */
    static std::vector<std::uint8_t> compress(const std::vector<std::uint8_t>& input, const Config& config);

    /**
     * Decompresses one LZSS byte stream and checks the expected output size.
     */
    static bool decompress(const std::vector<std::uint8_t>& input, std::uint32_t expected_size, const Config& config, std::vector<std::uint8_t>& output);
};
