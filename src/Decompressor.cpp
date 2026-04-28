/**
 * @file Decompressor.cpp
 * @date 2026-04-28
 * @brief Implements the command-line decompression workflow.
 * @author TODO: fill in name, surname and login
 */

#include "Decompressor.hpp"

#include "HeaderProvider.hpp"
#include <fstream>
#include <iostream>

namespace {
// Temporary pass-through payload reader used until the real LZSS decoder exists.
bool copy_remaining_data(std::istream& input, std::ostream& output) {
    output << input.rdbuf();
    return output.good();
}

#ifdef DEBUG
void print_header_debug(const CodecHeader& header) {
    std::cout << "Cteni hlavicky:\n";
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

// Run the decompression workflow: read the codec header first so the decoder
// knows the original image parameters, then restore the payload.
int Decompressor::decompress(const ParsedArgs& args) {
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

#ifdef DEBUG
    print_header_debug(header);
#endif

    std::ofstream out(args.outfile, std::ios::binary);
    if (!out) {
        std::cerr << "Nelze otevřít výstupní soubor: " << args.outfile << "\n";
        return 1;
    }

    if (!copy_remaining_data(in, out)) {
        std::cerr << "Chyba při zápisu výstupního souboru: " << args.outfile << "\n";
        return 1;
    }

    return 0;
}
