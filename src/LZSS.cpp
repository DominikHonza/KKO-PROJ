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
/**
 * @brief Structure representing LZSS match.
 */
struct Match {
    std::uint16_t offset = 0;
    std::uint8_t length = 0;
};

/**
 * @brief Finds best match in history buffer.
 * @param input Input data.
 * @param position Current position.
 * @param config Compression configuration.
 * @return Best match (offset, length).
 */
Match find_best_match(const std::vector<std::uint8_t>& input, std::size_t position, const Config& config) {
    Match best;

    // Start of history window (limited by history_buffer_size)
    const std::size_t history_start = position > config.history_buffer_size ? position - config.history_buffer_size : 0;
    // Maximum match length limited by lookahead buffer and input end
    const std::size_t max_length = std::min<std::size_t>(config.lookahead_buffer_size, input.size() - position);

     // Try all possible candidates in history window
    for (std::size_t candidate = history_start; candidate < position; ++candidate) {
        std::size_t length = 0;
        // Compare bytes until mismatch or limit reached
        while (length < max_length && input[candidate + length] == input[position + length]) {
            ++length;
        }

          // Keep best match that satisfies minimum length
        if (length >= config.min_match_length && length > best.length) {
            best.offset = static_cast<std::uint16_t>(position - candidate); // Offset = distance backwards from current position
            best.length = static_cast<std::uint8_t>(length); // Length of matching sequence

            // Early exit if maximum possible match found
            if (length == max_length) {
                break;
            }
        }
    }

    return best;
}
}

/**
 * @brief Compresses input data using LZSS.
 * @param input Input data.
 * @param config Compression configuration.
 * @return Compressed byte stream.
 */
std::vector<std::uint8_t> LZSS::compress(const std::vector<std::uint8_t>& input, const Config& config) {
    std::vector<std::uint8_t> output;
    std::size_t position = 0;

    // Process input until all bytes are encoded
    while (position < input.size()) {
         // Reserve space for flag byte (will be filled later)
        const std::size_t flags_position = output.size();
        output.push_back(0);
        std::uint8_t flags = 0;

        // Each flag byte describes next 8 items
        for (std::uint8_t bit = 0; bit < 8 && position < input.size(); ++bit) {
            const Match match = find_best_match(input, position, config); // Find best match in history buffer

            if (match.length >= config.min_match_length) {
                // Set bit -> this is a reference (match)
                flags |= static_cast<std::uint8_t>(1u << bit);
                ByteIO::write_u16(output, match.offset);
                ByteIO::write_u8(output, match.length);
                position += match.length; // Skip matched sequence
            } else {
                ByteIO::write_u8(output, input[position]); // Store literal byte
                ++position;
            }
        }

        output[flags_position] = flags;
    }

    return output;
}

/**
 * @brief Decompresses LZSS encoded data.
 * @param input Compressed input data.
 * @param expected_size Expected output size.
 * @param config Compression configuration.
 * @param output Output buffer.
 * @return true on success, false otherwise.
 */
bool LZSS::decompress(const std::vector<std::uint8_t>& input, std::uint32_t expected_size, const Config& config, std::vector<std::uint8_t>& output) {
    (void)config; // config is used only for validation (min_match_length)
    output.clear();
    output.reserve(expected_size);

     // Process input until fully consumed or expected output size reached
    std::size_t position = 0;
    while (position < input.size() && output.size() < expected_size) {
        std::uint8_t flags = 0;

        // Each flag byte controls next 8 items (literal or match)
        if (!ByteIO::read_u8(input, position, flags)) {
            return false;
        }

        for (std::uint8_t bit = 0; bit < 8 && output.size() < expected_size; ++bit) {
            // Check current bit is 0 = literal, 1 = reference
            if ((flags & static_cast<std::uint8_t>(1u << bit)) == 0) {
                std::uint8_t literal = 0;
                if (!ByteIO::read_u8(input, position, literal)) {
                    return false;
                }
                output.push_back(literal);
            } else {

                // Read (offset, length) pair
                std::uint16_t offset = 0;
                std::uint8_t length = 0;
                if (!ByteIO::read_u16(input, position, offset) || !ByteIO::read_u8(input, position, length)) {
                    return false;
                }
                
                // Validate reference:
                // offset must point into already decoded data
                // length must be valid
                if (offset == 0 || offset > output.size() || length < config.min_match_length) {
                    return false;
                }

                // Copy referenced sequence from already decoded output - sliding window mechanism
                for (std::uint8_t i = 0; i < length; ++i) {
                    // Copy byte from distance offset backwards
                    output.push_back(output[output.size() - offset]);
                    // Prevent overflow beyond expected output size
                    if (output.size() > expected_size) {
                        return false;
                    }
                }
            }
        }
    }

    // Final validation:
    // all input consumed
    // output size matches expected
    return position == input.size() && output.size() == expected_size;
}
