/**
 * @file Compressor.cpp
 * @date 2026-04-28
 * @brief Implements the command-line compression workflow.
 * @author TODO: fill in name, surname and login
 */

#include "Compressor.hpp"

#include "ConfigProvider.hpp"
#include "HeaderProvider.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>

namespace {
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

// Temporary pass-through payload writer used until the real LZSS encoder exists.
bool copy_remaining_data(std::istream& input, std::ostream& output) {
    output << input.rdbuf();
    return output.good();
}

// Static scanning stores one logical block; adaptive scanning stores a block grid.
std::uint32_t calculate_block_count(
    const Config& config,
    std::uint32_t width,
    std::uint32_t height,
    bool adaptive_scan
) {
    if (!adaptive_scan) {
        return 1;
    }

    return (width / config.block_dimension) * (height / config.block_dimension);
}

#ifdef DEBUG
void print_header_debug(const CodecHeader& header) {
    std::cout << "Zapis hlavicky:\n";
    std::cout << "  Model: " << (header.use_model ? "aktivni" : "neaktivni") << "\n";
    std::cout << "  Skenovani: " << (header.adaptive_scan ? "adaptivni" : "sekvencni") << "\n";
    std::cout << "  Sirka: " << header.width << "\n";
    std::cout << "  Vyska: " << header.height << "\n";
    std::cout << "  Puvodni velikost: " << header.original_size << "\n";
    std::cout << "  Pocet bloku: " << header.block_count << "\n";
    std::cout << "  Velikost bloku: " << header.config.block_dimension << "\n";
    std::cout << "  Historie LZSS: " << header.config.history_buffer_size << "\n";
    std::cout << "  Lookahead LZSS: " << static_cast<int>(header.config.lookahead_buffer_size) << "\n";
    std::cout << "  Minimalni shoda: " << static_cast<int>(header.config.min_match_length) << "\n";
}
#endif
}

// Run the compression workflow: validate RAW input, write the codec header,
// and then write the payload. The payload is currently copied unchanged.
int Compressor::compress(const ParsedArgs& args) {
    std::ifstream in(args.infile, std::ios::binary);
    if (!in) {
        std::cerr << "Nelze otevřít vstupní soubor: " << args.infile << "\n";
        return 1;
    }

    Config config = ConfigProvider::get_default();

    std::uint64_t input_size = 0;
    if (!get_input_size(in, input_size)) {
        std::cerr << "Nelze zjistit velikost vstupního souboru: " << args.infile << "\n";
        return 1;
    }

    std::string error;
    if (!ConfigProvider::validate_for_compression(
            config,
            args.width,
            input_size,
            args.adaptive_scan,
            error
        )) {
        std::cerr << error << "\n";
        return 1;
    }

    std::ofstream out(args.outfile, std::ios::binary);
    if (!out) {
        std::cerr << "Nelze otevřít výstupní soubor: " << args.outfile << "\n";
        return 1;
    }

    if (input_size > std::numeric_limits<std::uint32_t>::max()) {
        std::cerr << "Nevalidni vstup: vstupni soubor je prilis velky pro aktualni format hlavicky.\n";
        return 1;
    }

    const auto width = static_cast<std::uint32_t>(args.width);
    const auto height = static_cast<std::uint32_t>(input_size / width);

    // Store every parameter that decompression must know without CLI flags.
    CodecHeader header;
    header.use_model = args.use_model;
    header.adaptive_scan = args.adaptive_scan;
    header.width = width;
    header.height = height;
    header.original_size = static_cast<std::uint32_t>(input_size);
    header.block_count = calculate_block_count(config, width, height, args.adaptive_scan);
    header.config = config;

#ifdef DEBUG
    print_header_debug(header);
#endif

    if (!HeaderProvider::write_header(out, header, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    if (!copy_remaining_data(in, out)) {
        std::cerr << "Chyba při zápisu výstupního souboru: " << args.outfile << "\n";
        return 1;
    }

    return 0;
}
