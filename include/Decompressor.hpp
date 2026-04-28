/**
 * @file Decompressor.hpp
 * @date 2026-04-28
 * @brief Declares the decompression entry point.
 * @author Dominik Honza xhonza04
 */

#pragma once

#include "args.hpp"
#include "HeaderProvider.hpp"
#include <cstdint>
#include <iosfwd>
#include <vector>

/**
 * Handles the decompression mode selected from the command line.
 */
class Decompressor {
public:
    /**
     * Executes the full decompression workflow for one input file.
     *
     * This method owns file opening, header reading and output error handling.
     * The actual payload decompression is delegated to the private decompress
     * method.
     *
     * @param args Parsed command-line arguments for decompression.
     * @return Process exit code, where 0 means success.
     */
    static int execute(const ParsedArgs& args);

private:
    /**
     * Payload block read from the compressed stream.
     */
    struct EncodedBlock {
        bool compressed = false;
        bool vertical_scan = false;
        std::uint32_t raw_size = 0;
        std::vector<std::uint8_t> payload;
    };

    /**
     * Decompresses already opened payload streams.
     *
     * The current implementation copies the payload unchanged. The LZSS decoder
     * will be added behind this method.
     *
     * @param input Binary compressed input stream positioned after header.
     * @param output Binary RAW output stream.
     * @return True on success, false on I/O failure.
     */
    static bool decompress(std::istream& input, std::ostream& output, const CodecHeader& header);

    /**
     * Reads one payload block including block metadata.
     */
    static bool read_block(std::istream& input, EncodedBlock& block);

    /**
     * Writes one already decoded block to the RAW output stream.
     */
    static bool write_block(const std::vector<std::uint8_t>& block_data, std::ostream& output);

    /**
     * Restores adaptively stored blocks into original raster order.
     */
    static bool write_adaptive_blocks(std::istream& input, std::ostream& output, const CodecHeader& header);

    /**
     * Reverses the optional predictive model after payload decompression.
     */
    static void apply_inverse_model(std::vector<std::uint8_t>& data, const CodecHeader& header);
};
