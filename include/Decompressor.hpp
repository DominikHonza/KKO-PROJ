/**
 * @file Decompressor.hpp
 * @date 2026-04-28
 * @brief Declares the decompression entry point.
 * @author TODO: fill in name, surname and login
 */

#pragma once

#include "args.hpp"

/**
 * Handles the decompression mode selected from the command line.
 */
class Decompressor {
public:
    /**
     * Decompresses the input file into the output RAW file.
     *
     * The current implementation reads and validates the codec header, then
     * copies the remaining payload unchanged. LZSS decoding will be added here.
     *
     * @param args Parsed command-line arguments for decompression.
     * @return Process exit code, where 0 means success.
     */
    static int decompress(const ParsedArgs& args);
};
