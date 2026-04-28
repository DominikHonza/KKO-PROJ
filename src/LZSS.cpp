/**
 * @file LZSS.cpp
 * @date 2026-04-28
 * @brief Implements a simple LZSS encoder and decoder.
 * @author Dominik Honza xhonza04
 */

#include "LZSS.hpp"
#include "ByteIO.hpp"

#include <algorithm>
#include <cstddef>

namespace {
struct Match {
    std::uint16_t offset = 0;
    std::uint8_t length = 0;
};

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
                ByteIO::write_u16(output, match.offset);
                ByteIO::write_u8(output, match.length);
                position += match.length;
            } else {
                ByteIO::write_u8(output, input[position]);
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
        if (!ByteIO::read_u8(input, position, flags)) {
            return false;
        }

        for (std::uint8_t bit = 0; bit < 8 && output.size() < expected_size; ++bit) {
            if ((flags & static_cast<std::uint8_t>(1u << bit)) == 0) {
                std::uint8_t literal = 0;
                if (!ByteIO::read_u8(input, position, literal)) {
                    return false;
                }
                output.push_back(literal);
            } else {
                std::uint16_t offset = 0;
                std::uint8_t length = 0;
                if (!ByteIO::read_u16(input, position, offset) || !ByteIO::read_u8(input, position, length)) {
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
