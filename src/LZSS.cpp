/**
 * @file LZSS.cpp
 * @date 2026-04-28
 * @brief Implements a simple LZSS encoder and decoder.
 * @author Dominik Honza xhonza04
 */

#include "LZSS.hpp"

#include <algorithm>
#include <cstddef>

namespace {
struct Match {
    std::uint16_t offset = 0;
    std::uint8_t length = 0;
};

bool write_u8(std::vector<std::uint8_t>& output, std::uint8_t value) {
    output.push_back(value);
    return true;
}

bool write_u16(std::vector<std::uint8_t>& output, std::uint16_t value) {
    output.push_back(static_cast<std::uint8_t>(value & 0xffu));
    output.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
    return true;
}

bool read_u8(const std::vector<std::uint8_t>& input, std::size_t& position, std::uint8_t& value) {
    if (position >= input.size()) {
        return false;
    }

    value = input[position++];
    return true;
}

bool read_u16(const std::vector<std::uint8_t>& input, std::size_t& position, std::uint16_t& value) {
    std::uint8_t b0 = 0;
    std::uint8_t b1 = 0;

    if (!read_u8(input, position, b0) || !read_u8(input, position, b1)) {
        return false;
    }

    value = static_cast<std::uint16_t>(b0) | static_cast<std::uint16_t>(static_cast<std::uint16_t>(b1) << 8u);
    return true;
}

Match find_best_match(const std::vector<std::uint8_t>& input, std::size_t position, const Config& config) {
    Match best;
    const std::size_t history_start = position > config.history_buffer_size ? position - config.history_buffer_size : 0;
    const std::size_t max_length = std::min<std::size_t>(config.lookahead_buffer_size, input.size() - position);

    for (std::size_t candidate = history_start; candidate < position; ++candidate) {
        std::size_t length = 0;
        while (length < max_length && input[candidate + length] == input[position + length]) {
            ++length;
        }

        if (length >= config.min_match_length && length > best.length) {
            best.offset = static_cast<std::uint16_t>(position - candidate);
            best.length = static_cast<std::uint8_t>(length);

            if (length == max_length) {
                break;
            }
        }
    }

    return best;
}
}

std::vector<std::uint8_t> LZSS::compress(const std::vector<std::uint8_t>& input, const Config& config) {
    std::vector<std::uint8_t> output;
    std::size_t position = 0;

    while (position < input.size()) {
        const std::size_t flags_position = output.size();
        output.push_back(0);
        std::uint8_t flags = 0;

        for (std::uint8_t bit = 0; bit < 8 && position < input.size(); ++bit) {
            const Match match = find_best_match(input, position, config);

            if (match.length >= config.min_match_length) {
                flags |= static_cast<std::uint8_t>(1u << bit);
                write_u16(output, match.offset);
                write_u8(output, match.length);
                position += match.length;
            } else {
                write_u8(output, input[position]);
                ++position;
            }
        }

        output[flags_position] = flags;
    }

    return output;
}

bool LZSS::decompress(const std::vector<std::uint8_t>& input, std::uint32_t expected_size, const Config& config, std::vector<std::uint8_t>& output) {
    (void)config;
    output.clear();
    output.reserve(expected_size);

    std::size_t position = 0;
    while (position < input.size() && output.size() < expected_size) {
        std::uint8_t flags = 0;
        if (!read_u8(input, position, flags)) {
            return false;
        }

        for (std::uint8_t bit = 0; bit < 8 && output.size() < expected_size; ++bit) {
            if ((flags & static_cast<std::uint8_t>(1u << bit)) == 0) {
                std::uint8_t literal = 0;
                if (!read_u8(input, position, literal)) {
                    return false;
                }
                output.push_back(literal);
            } else {
                std::uint16_t offset = 0;
                std::uint8_t length = 0;
                if (!read_u16(input, position, offset) || !read_u8(input, position, length)) {
                    return false;
                }

                if (offset == 0 || offset > output.size() || length < config.min_match_length) {
                    return false;
                }

                for (std::uint8_t i = 0; i < length; ++i) {
                    output.push_back(output[output.size() - offset]);
                    if (output.size() > expected_size) {
                        return false;
                    }
                }
            }
        }
    }

    return position == input.size() && output.size() == expected_size;
}
