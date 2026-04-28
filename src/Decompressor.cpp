/**
 * @file Decompressor.cpp
 * @date 2026-04-28
 * @brief Implements the command-line decompression workflow.
 * @author Dominik Honza xhonza04
 */

#include "Decompressor.hpp"
#include "ByteIO.hpp"
#include "HeaderProvider.hpp"
#include "LZSS.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

namespace {
constexpr std::uint8_t BLOCK_FLAG_COMPRESSED = 1u << 0;
constexpr std::uint8_t BLOCK_FLAG_VERTICAL_SCAN = 1u << 1;
}

/**
 * @brief Executes decompression workflow.
 * @param args Parsed CLI arguments.
 * @return Exit code (0 success, non-zero error).
 */
int Decompressor::execute(const ParsedArgs& args) {
    std::ifstream in(args.infile, std::ios::binary);
    if (!in) {
        std::cerr << "Nelze otevřít vstupní soubor: " << args.infile << "\n";
        return 1;
    }

    CodecHeader header;
    std::string error;
    if (!HeaderProvider::read_header(in, header, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    std::ofstream out(args.outfile, std::ios::binary);
    if (!out) {
        std::cerr << "Nelze otevřít výstupní soubor: " << args.outfile << "\n";
        return 1;
    }

    if (!decompress(in, out, header)) {
        std::cerr << "Chyba při zápisu výstupního souboru: " << args.outfile << "\n";
        return 1;
    }

    return 0;
}

/**
 * @brief Decompresses input stream into output stream.
 * @param input Input stream.
 * @param output Output stream.
 * @param header Codec header.
 * @return true on success, false otherwise.
 */
bool Decompressor::decompress(std::istream& input, std::ostream& output, const CodecHeader& header) {
    // Adaptive mode uses block reconstruction logic
    if (header.adaptive_scan) {
        return write_adaptive_blocks(input, output, header);
    }

    // Process all blocks
    for (std::uint32_t i = 0; i < header.block_count; ++i) {
        EncodedBlock block;

        // Process all blocks (static mode = usually 1 block)
        if (!read_block(input, block)) {
            return false;
        }

        // Decompress payload if LZSS was used
        if (block.compressed) {
            std::vector<std::uint8_t> decoded_payload;
            if (!LZSS::decompress(block.payload, block.raw_size, header.config, decoded_payload)) {
                return false;
            }
            block.payload = decoded_payload;
        }

        // Validate raw (uncompressed) block size
        if (!block.compressed && block.payload.size() != block.raw_size) {
            return false;
        }

        apply_inverse_model(block.payload, header);

        if (!write_block(block.payload, output)) {
            return false;
        }
    }

    return output.good();
}

/**
 * @brief Decompresses adaptive blocks and reconstructs full image.
 * @param input Input stream.
 * @param output Output stream.
 * @param header Codec header.
 * @return true on success, false otherwise.
 */
bool Decompressor::write_adaptive_blocks(std::istream& input, std::ostream& output, const CodecHeader& header) {
    const std::uint32_t block_dimension = header.config.block_dimension;
    // Allocate full output image buffer
    std::vector<std::uint8_t> image(header.original_size);

    for (std::uint32_t block_y = 0; block_y < header.height; block_y += block_dimension) {
        for (std::uint32_t block_x = 0; block_x < header.width; block_x += block_dimension) {
            EncodedBlock block;
            if (!read_block(input, block)) {
                return false;
            }
            
             // Decompress if needed
            const std::uint32_t expected_size = block_dimension * block_dimension;
            if (block.compressed) {
                std::vector<std::uint8_t> decoded_payload;
                if (!LZSS::decompress(block.payload, block.raw_size, header.config, decoded_payload)) {
                    return false;
                }
                block.payload = decoded_payload;
            }

            // Validate payload size for raw blocks
            if (!block.compressed && block.payload.size() != block.raw_size) {
                return false;
            }

            // Each block must match expected block size
            if (block.raw_size != expected_size) {
                return false;
            }

            // Reverse difference model (if used)
            apply_inverse_model(block.payload, header);

            // Reconstruct block into image buffer
            std::size_t source_index = 0;
            if (block.vertical_scan) {
                for (std::uint32_t x = 0; x < block_dimension; ++x) {
                    for (std::uint32_t y = 0; y < block_dimension; ++y) {
                        image[(block_y + y) * header.width + block_x + x] = block.payload[source_index++];
                    }
                }
            } else {
                for (std::uint32_t y = 0; y < block_dimension; ++y) {
                    const std::uint32_t row_start = (block_y + y) * header.width + block_x;
                    for (std::uint32_t x = 0; x < block_dimension; ++x) {
                        image[row_start + x] = block.payload[source_index++];
                    }
                }
            }
        }
    }

    return write_block(image, output);
}

/**
 * @brief Applies inverse difference model.
 * @param data Input/output data.
 * @param header Codec header.
 */
void Decompressor::apply_inverse_model(std::vector<std::uint8_t>& data, const CodecHeader& header) {
    if (!header.use_model || data.empty()) {
        return;
    }

    for (std::size_t i = 1; i < data.size(); ++i) {
        data[i] = static_cast<std::uint8_t>((data[i] + data[i - 1]) % 256);
    }
}

/**
 * @brief Reads encoded block from input stream.
 * @param input Input stream.
 * @param block Output block structure.
 * @return true on success, false otherwise.
 */
bool Decompressor::read_block(std::istream& input, EncodedBlock& block) {
    std::uint8_t flags = 0;
    std::uint32_t payload_size = 0;

    // Read block metadata: flags, original size, payload size
    if (!ByteIO::read_u8(input, flags) || !ByteIO::read_u32(input, block.raw_size) || !ByteIO::read_u32(input, payload_size)) {
        return false;
    }

    // Decode flags:
    // compressed → whether LZSS was applied
    // vertical_scan → scan direction used during compression
    block.compressed = (flags & BLOCK_FLAG_COMPRESSED) != 0;
    block.vertical_scan = (flags & BLOCK_FLAG_VERTICAL_SCAN) != 0;

    // Validate flags -> reject unknown bits (corrupted input)
    const std::uint8_t known_flags = BLOCK_FLAG_COMPRESSED | BLOCK_FLAG_VERTICAL_SCAN;
    if ((flags & ~known_flags) != 0) {
        return false;
    }

    // Allocate buffer for payload
    block.payload.resize(payload_size);
    // If no payload, block is valid (empty)
    if (payload_size == 0) {
        return true;
    }

    // Read payload bytes (raw or compressed data)
    input.read(reinterpret_cast<char*>(block.payload.data()), static_cast<std::streamsize>(block.payload.size()));
    return input.gcount() == static_cast<std::streamsize>(block.payload.size());
}

/**
 * @brief Writes raw block data to output stream.
 * @param block_data Data to write.
 * @param output Output stream.
 * @return true on success, false otherwise.
 */
bool Decompressor::write_block(const std::vector<std::uint8_t>& block_data, std::ostream& output) {
    if (!block_data.empty()) {
        output.write(reinterpret_cast<const char*>(block_data.data()), static_cast<std::streamsize>(block_data.size()));
    }

    return output.good();
}
