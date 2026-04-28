/**
 * @file Compressor.cpp
 * @date 2026-04-28
 * @brief Implements the command-line compression workflow.
 * @author Dominik Honza xhonza04
 */

#include "Compressor.hpp"
#include "ConfigProvider.hpp"
#include "HeaderProvider.hpp"
#include "ByteIO.hpp"
#include "LZSS.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

namespace {
constexpr std::uint8_t BLOCK_FLAG_COMPRESSED = 1u << 0;
constexpr std::uint8_t BLOCK_FLAG_VERTICAL_SCAN = 1u << 1;
constexpr std::uint32_t BLOCK_METADATA_SIZE = 9;

// Determine RAW input size and rewind the stream for later payload processing.
bool get_input_size(std::ifstream& input, std::uint64_t& input_size) {
    input.seekg(0, std::ios::end);
    const auto position = input.tellg();
    if (position < 0) {
        return false;
    }

    input_size = static_cast<std::uint64_t>(position);
    input.seekg(0, std::ios::beg);
    return input.good();
}

// Static scanning stores one logical block; adaptive scanning stores a block grid.
std::uint32_t calculate_block_count(const Config& config, std::uint32_t width, std::uint32_t height, bool adaptive_scan) {
    if (!adaptive_scan) {
        return 1;
    }

    return (width / config.block_dimension) * (height / config.block_dimension);
}
}

bool Compressor::read_raw_data(std::istream& input, std::vector<std::uint8_t>& raw_data, std::uint32_t expected_size) {
    raw_data.resize(expected_size);

    if (expected_size == 0) {
        return true;
    }

    input.read(reinterpret_cast<char*>(raw_data.data()), static_cast<std::streamsize>(raw_data.size()));

    return input.gcount() == static_cast<std::streamsize>(raw_data.size());
}

// Run the compression workflow: validate RAW input, write the codec header,
// and then delegate payload processing to Compressor::compress.
int Compressor::execute(const ParsedArgs& args) {

    // Check if input file can be opened
    std::ifstream in(args.infile, std::ios::binary);
    if (!in) {
        std::cerr << "Nelze otevřít vstupní soubor: " << args.infile << "\n";
        return 1;
    }

    // Init config
    Config config = ConfigProvider::get_default();

    // Load input size for later comparsion with comprimed
    std::uint64_t input_size = 0;
    if (!get_input_size(in, input_size)) {
        std::cerr << "Nelze zjistit velikost vstupního souboru: " << args.infile << "\n";
        return 1;
    }

    // Validate setup parameters for LZ compression method
    std::string error;
    if (!ConfigProvider::validate_for_compression(config, args.width, input_size, args.adaptive_scan, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    // Check if output file is ok
    std::ofstream out(args.outfile, std::ios::binary);
    if (!out) {
        std::cerr << "Nelze otevřít výstupní soubor: " << args.outfile << "\n";
        return 1;
    }

    // Sanity check for input size
    if (input_size > std::numeric_limits<std::uint32_t>::max()) {
        std::cerr << "Nevalidni vstup: vstupni soubor je prilis velky pro aktualni format hlavicky.\n";
        return 1;
    }

    // Calc dimensions of input
    const auto width = static_cast<std::uint32_t>(args.width);
    const auto height = static_cast<std::uint32_t>(input_size / width);

    // Store every parameter that decompression must know without CLI flags.
    // This header is then saved to compressed file
    CodecHeader header;
    header.use_model = args.use_model;
    header.adaptive_scan = args.adaptive_scan;
    header.width = width;
    header.height = height;
    header.original_size = static_cast<std::uint32_t>(input_size);
    header.block_count = calculate_block_count(config, width, height, args.adaptive_scan);
    header.config = config;

    // Writes header to output file
    if (!HeaderProvider::write_header(out, header, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    // Reads data for upcoming compression
    std::vector<std::uint8_t> raw_data;
    if (!read_raw_data(in, raw_data, header.original_size)) {
        std::cerr << "Chyba při čtení vstupního souboru: " << args.infile << "\n";
        return 1;
    }

    // Main body of compression method execution
    if (!compress(raw_data, out, header)) {
        std::cerr << "Chyba při zápisu výstupního souboru: " << args.outfile << "\n";
        return 1;
    }

    return 0;
}

// Select the image processing path according to switches stored in the header.
bool Compressor::compress(const std::vector<std::uint8_t>& raw_data, std::ostream& output, const CodecHeader& header) {
    if (header.adaptive_scan) {
        return compress_adaptive(raw_data, output, header);
    }else{
        return compress_static(raw_data, output, header);
    }
}

// Static mode keeps the whole image as one horizontal byte stream.
bool Compressor::compress_static(const std::vector<std::uint8_t>& raw_data, std::ostream& output, const CodecHeader& header) {
    const EncodedBlock block = encode_block(raw_data, header, false);
    return write_block(block, output);
}

// Adaptive mode will later split the image into blocks and choose scan order.
bool Compressor::compress_adaptive(const std::vector<std::uint8_t>& raw_data, std::ostream& output, const CodecHeader& header) {
    const std::uint32_t block_dimension = header.config.block_dimension;

    for (std::uint32_t block_y = 0; block_y < header.height; block_y += block_dimension) {
        for (std::uint32_t block_x = 0; block_x < header.width; block_x += block_dimension) {
            const EncodedBlock horizontal_block = encode_block(scan_block_horizontal(raw_data, header, block_x, block_y), header, false);
            const EncodedBlock vertical_block = encode_block(scan_block_vertical(raw_data, header, block_x, block_y), header, true);
            const std::uint32_t horizontal_size = BLOCK_METADATA_SIZE + static_cast<std::uint32_t>(horizontal_block.payload.size());
            const std::uint32_t vertical_size = BLOCK_METADATA_SIZE + static_cast<std::uint32_t>(vertical_block.payload.size());
            const EncodedBlock& block = vertical_size < horizontal_size ? vertical_block : horizontal_block;

            if (!write_block(block, output)) {
                return false;
            }
        }
    }

    return output.good();
}

std::vector<std::uint8_t> Compressor::scan_block_horizontal(const std::vector<std::uint8_t>& raw_data, const CodecHeader& header, std::uint32_t block_x, std::uint32_t block_y) {
    const std::uint32_t block_dimension = header.config.block_dimension;
    std::vector<std::uint8_t> block_data;
    block_data.reserve(static_cast<std::size_t>(block_dimension) * block_dimension);

    for (std::uint32_t y = 0; y < block_dimension; ++y) {
        const std::uint32_t row_start = (block_y + y) * header.width + block_x;
        for (std::uint32_t x = 0; x < block_dimension; ++x) {
            block_data.push_back(raw_data[row_start + x]);
        }
    }

    return block_data;
}

std::vector<std::uint8_t> Compressor::scan_block_vertical(const std::vector<std::uint8_t>& raw_data, const CodecHeader& header, std::uint32_t block_x, std::uint32_t block_y) {
    const std::uint32_t block_dimension = header.config.block_dimension;
    std::vector<std::uint8_t> block_data;
    block_data.reserve(static_cast<std::size_t>(block_dimension) * block_dimension);

    for (std::uint32_t x = 0; x < block_dimension; ++x) {
        for (std::uint32_t y = 0; y < block_dimension; ++y) {
            block_data.push_back(raw_data[(block_y + y) * header.width + block_x + x]);
        }
    }

    return block_data;
}

// Applys differernce model if arg is on
void Compressor::apply_model(std::vector<std::uint8_t>& data, const CodecHeader& header) {
    if (!header.use_model || data.empty()) {
        return;
    }

    // Applying difference model, then modulo using 
    std::uint8_t previous = data[0];
    for (std::size_t i = 1; i < data.size(); ++i) {
        const std::uint8_t current = data[i];
        data[i] = static_cast<std::uint8_t>((256 + current - previous) % 256); // Normalize
        previous = current;
    }
}

// Temporary pass-through block writer used until block metadata and LZSS exist.
bool Compressor::write_block(const EncodedBlock& block, std::ostream& output) {
    std::uint8_t flags = 0;

    // Set flags for if compression was better than raw and then used
    if (block.compressed) {
        flags |= BLOCK_FLAG_COMPRESSED;
    }

    // Set if direction was switched to vertical
    if (block.vertical_scan) {
        flags |= BLOCK_FLAG_VERTICAL_SCAN;
    }

    // Writes flags how was compression performed and metadata
    if (!ByteIO::write_u8(output, flags) || !ByteIO::write_u32(output, block.raw_size) || !ByteIO::write_u32(output, static_cast<std::uint32_t>(block.payload.size()))) {
        return false;
    }

    // Compressed payload
    if (!block.payload.empty()) {
        output.write(reinterpret_cast<const char*>(block.payload.data()), static_cast<std::streamsize>(block.payload.size()));
    }

    return output.good();
}

// MAIN LZSS method, compressed block of data
Compressor::EncodedBlock Compressor::encode_block(const std::vector<std::uint8_t>& block_data, const CodecHeader& header, bool vertical_scan) {
    EncodedBlock block;
    block.compressed = false;
    block.vertical_scan = vertical_scan;
    block.raw_size = static_cast<std::uint32_t>(block_data.size());
    block.payload = block_data;

    apply_model(block.payload, header);
    const std::vector<std::uint8_t> compressed_payload = LZSS::compress(block.payload, header.config);

    if (compressed_payload.size() < block.payload.size()) {
        block.compressed = true;
        block.payload = compressed_payload;
    }

    return block;
}