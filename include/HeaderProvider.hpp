/**
 * @file HeaderProvider.hpp
 * @date 2026-04-28
 * @brief Declares helpers for reading and writing the LZSS file header.
 * @author Dominik Honza xhonza04
 */

#pragma once

#include "ConfigProvider.hpp"
#include <cstdint>
#include <iosfwd>
#include <string>

/**
 * Metadata stored at the beginning of every compressed file.
 *
 * The decoder uses this structure to reconstruct the original image without
 * command-line parameters such as width, model selection, or scan mode.
 */
struct CodecHeader {
    /** True when the predictive model was applied before LZSS compression. */
    bool use_model = false;

    /** True when image data was split into blocks with adaptive scan order. */
    bool adaptive_scan = false;

    /** Original image width in pixels. */
    std::uint32_t width = 0;

    /** Original image height in pixels. */
    std::uint32_t height = 0;

    /** Original RAW input size in bytes. For 8-bit grayscale it equals width * height. */
    std::uint32_t original_size = 0;

    /** Number of serialized data blocks stored after the file header. */
    std::uint32_t block_count = 0;

    /** Compression parameters needed by both encoder and decoder. */
    Config config;
};

/**
 * Provides a portable binary representation of CodecHeader.
 *
 * Values are written field-by-field in little-endian order instead of dumping
 * a C++ structure directly. This avoids padding and platform-dependence.
 */
class HeaderProvider {
public:
    /**
     * Validates and writes the file header to the output stream.
     *
     * @param output Binary output stream positioned at the beginning of the file.
     * @param header Header values that describe the compressed payload.
     * @param error Receives a human-readable message when writing fails.
     * @return True on success, false on validation or I/O failure.
     */
    static bool write_header(
        std::ostream& output,
        const CodecHeader& header,
        std::string& error
    );

    /**
     * Reads and validates the file header from the input stream.
     *
     * @param input Binary input stream positioned at the beginning of the file.
     * @param header Receives decoded header values.
     * @param error Receives a human-readable message when reading fails.
     * @return True on success, false when the file is not a valid LZSS stream.
     */
    static bool read_header(
        std::istream& input,
        CodecHeader& header,
        std::string& error
    );

private:
    /** File signature used to reject non-LZSS input before decoding payload data. */
    static constexpr char MAGIC[4] = {'L', 'Z', 'S', 'S'};

    /** Header flag bit telling the decoder to run the inverse model step. */
    static constexpr std::uint8_t FLAG_MODEL = 1u << 0;

    /** Header flag bit telling the decoder to restore data from block scan order. */
    static constexpr std::uint8_t FLAG_ADAPTIVE_SCAN = 1u << 1;
};
