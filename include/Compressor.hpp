/**
 * @file Compressor.hpp
 * @date 2026-04-28
 * @brief Declares the compression entry point.
 * @author TODO: fill in name, surname and login
 */

#pragma once

#include "args.hpp"

/**
 * Handles the compression mode selected from the command line.
 */
class Compressor {
public:
    /**
     * Compresses the input file into the output file.
     *
     * The current implementation writes the codec header and stores the input
     * payload unchanged. The LZSS encoder will be added behind this interface.
     *
     * @param args Parsed command-line arguments for compression.
     * @return Process exit code, where 0 means success.
     */
    static int compress(const ParsedArgs& args);
};
