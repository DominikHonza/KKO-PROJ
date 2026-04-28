/**
 * @file Compressor.hpp
 * @date 2026-04-28
 * @brief Declares the compression entry point.
 * @author Dominik Honza xhonza04
 */

#pragma once

#include "args.hpp"
#include "HeaderProvider.hpp"
#include <cstdint>
#include <iosfwd>
#include <vector>

/**
 * Handles the compression mode selected from the command line.
 */
class Compressor {
public:
    /**
     * Executes the full compression workflow for one input file.
     *
     * This method owns file opening, input validation, header creation and
     * output error handling. The actual payload processing is delegated to
     * private methods selected by command-line switches.
     *
     * @param args Parsed command-line arguments for compression.
     * @return Process exit code, where 0 means success.
     */
    static int execute(const ParsedArgs& args);

private:
    /**
     * Payload block prepared for writing into the compressed stream.
     */
    struct EncodedBlock {
        bool compressed = false;
        bool vertical_scan = false;
        std::uint32_t raw_size = 0;
        std::vector<std::uint8_t> payload;
    };

    /**
     * Compresses already loaded RAW image data.
     *
     * This method selects static or adaptive image processing according to the
     * header. The LZSS encoder will be added behind this method.
     *
     * @param raw_data Original RAW image data loaded from input.
     * @param output Binary compressed output stream positioned after header.
     * @param header Header describing image dimensions and selected switches.
     * @return True on success, false on I/O failure.
     */
    static bool compress(const std::vector<std::uint8_t>& raw_data, std::ostream& output, const CodecHeader& header);

    /**
     * Reads the whole RAW payload from the input stream.
     */
    static bool read_raw_data(std::istream& input, std::vector<std::uint8_t>& raw_data, std::uint32_t expected_size);

    /**
     * Processes the whole image as one horizontal stream.
     */
    static bool compress_static(const std::vector<std::uint8_t>& raw_data, std::ostream& output, const CodecHeader& header);

    /**
     * Processes the image as a grid of blocks. Real adaptive scan selection
     * will be implemented here later.
     */
    static bool compress_adaptive(const std::vector<std::uint8_t>& raw_data, std::ostream& output, const CodecHeader& header);

    /**
     * Applies preprocessing and chooses compressed/raw block representation.
     */
    static EncodedBlock encode_block(const std::vector<std::uint8_t>& block_data, const CodecHeader& header, bool vertical_scan);

    /**
     * Applies the optional predictive model before LZSS compression.
     */
    static void apply_model(std::vector<std::uint8_t>& data, const CodecHeader& header);

    /**
     * Writes one processed block to the compressed stream.
     */
    static bool write_block(const EncodedBlock& block, std::ostream& output);
};
