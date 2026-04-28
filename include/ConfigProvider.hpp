/**
 * @file ConfigProvider.hpp
 * @date 2026-04-28
 * @brief Declares default LZSS configuration and validation helpers.
 * @author Dominik Honza xhonza04
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * Tunable parameters shared by the encoder, decoder, and file header.
 */
struct Config {
    /** Side length of square image blocks used by adaptive scanning. */
    uint16_t block_dimension = 32;

    /** Maximum number of previous bytes searched by LZSS. */
    uint16_t history_buffer_size = 4096;

    /** Maximum number of upcoming bytes considered for a match. */
    uint8_t lookahead_buffer_size = 255;

    /** Minimum match length worth encoding as an LZSS reference. */
    uint8_t min_match_length = 3;
};

/**
 * Central place for distributing and validating compression parameters.
 */
class ConfigProvider {
public:
    /**
     * Returns the default configuration used by the compressor.
     */
    static Config get_default();

    /**
     * Checks internal consistency of compression parameters.
     *
     * @param config Configuration to validate.
     * @param error Receives a human-readable validation error.
     * @return True when the configuration can be used safely.
     */
    static bool validate(const Config& config, std::string& error);

    /**
     * Checks configuration together with RAW image properties.
     *
     * @param config Configuration to validate.
     * @param image_width Width passed by the user during compression.
     * @param input_size Size of the input RAW file in bytes.
     * @param adaptive_scan True when block-based adaptive scanning is enabled.
     * @param error Receives a human-readable validation error.
     * @return True when the image can be compressed with the given parameters.
     */
    static bool validate_for_compression(const Config& config, int image_width, std::uint64_t input_size, bool adaptive_scan, std::string& error);
};
