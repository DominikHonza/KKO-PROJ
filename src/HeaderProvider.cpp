/**
 * @file HeaderProvider.cpp
 * @date 2026-04-28
 * @brief Implements portable serialization of the LZSS file header.
 * @author Dominik Honza xhonza04
 */

#include "HeaderProvider.hpp"
#include "ByteIO.hpp"

#include <algorithm>
#include <array>
#include <istream>
#include <iterator>
#include <ostream>

namespace {

// Validate relationships that the decoder relies on while rebuilding pixels.
bool validate_header(const CodecHeader& header, std::string& error) {
    if (header.width == 0) {
        error = "Nevalidni hlavicka: sirka obrazu musi byt vetsi nez 0.";
        return false;
    }

    if (header.height == 0) {
        error = "Nevalidni hlavicka: vyska obrazu musi byt vetsi nez 0.";
        return false;
    }

    if (header.original_size == 0) {
        error = "Nevalidni hlavicka: puvodni velikost musi byt vetsi nez 0.";
        return false;
    }

    if (static_cast<std::uint64_t>(header.width) * header.height != header.original_size) {
        error = "Nevalidni hlavicka: rozmery obrazu neodpovidaji puvodni velikosti.";
        return false;
    }

    if (!ConfigProvider::validate(header.config, error)) {
        return false;
    }

    // Static mode serializes the whole image as one continuous horizontal block.
    if (!header.adaptive_scan) {
        if (header.block_count != 1) {
            error = "Nevalidni hlavicka: staticke skenovani musi obsahovat prave jeden blok.";
            return false;
        }

        return true;
    }

    // Adaptive mode uses a regular block grid, so dimensions must match it.
    if (header.width % header.config.block_dimension != 0 || header.height % header.config.block_dimension != 0) {
        error = "Nevalidni hlavicka: block_dimension musi delit sirku i vysku obrazu.";
        return false;
    }

    const std::uint32_t expected_block_count = (header.width / header.config.block_dimension) * (header.height / header.config.block_dimension);

    if (header.block_count != expected_block_count) {
        error = "Nevalidni hlavicka: pocet bloku neodpovida rozmerum obrazu.";
        return false;
    }

    return true;
}
}

// Validate the metadata, write the magic signature, and serialize all fields
// in the fixed order expected by the decoder.
bool HeaderProvider::write_header(std::ostream& output, const CodecHeader& header, std::string& error) {
    if (!validate_header(header, error)) {
        return false;
    }

    output.write(MAGIC, sizeof(MAGIC));
    if (!output.good()) {
        error = "Chyba pri zapisu hlavicky.";
        return false;
    }

    std::uint8_t flags = 0;
    if (header.use_model) {
        flags |= FLAG_MODEL;
    }
    if (header.adaptive_scan) {
        flags |= FLAG_ADAPTIVE_SCAN;
    }

    if (!ByteIO::write_u8(output, flags) || !ByteIO::write_u32(output, header.width) || !ByteIO::write_u32(output, header.height) || !ByteIO::write_u32(output, header.original_size) || !ByteIO::write_u16(output, header.config.block_dimension) || !ByteIO::write_u16(output, header.config.history_buffer_size) || !ByteIO::write_u8(output, header.config.lookahead_buffer_size) || !ByteIO::write_u8(output, header.config.min_match_length) || !ByteIO::write_u32(output, header.block_count)) {
        error = "Chyba pri zapisu hlavicky.";
        return false;
    }

    return true;
}

// Read the magic signature first, reject unknown input, then deserialize and
// validate the remaining header fields before payload decoding starts.
bool HeaderProvider::read_header(std::istream& input, CodecHeader& header, std::string& error) {
    std::array<char, sizeof(MAGIC)> magic{};
    if (!input.read(magic.data(), static_cast<std::streamsize>(magic.size()))) {
        error = "Nevalidni vstup: soubor neobsahuje celou hlavicku.";
        return false;
    }

    if (!std::equal(magic.begin(), magic.end(), std::begin(MAGIC))) {
        error = "Nevalidni vstup: soubor nema format LZSS.";
        return false;
    }

    std::uint8_t flags = 0;
    if (!ByteIO::read_u8(input, flags) || !ByteIO::read_u32(input, header.width) || !ByteIO::read_u32(input, header.height) || !ByteIO::read_u32(input, header.original_size) || !ByteIO::read_u16(input, header.config.block_dimension) || !ByteIO::read_u16(input, header.config.history_buffer_size) || !ByteIO::read_u8(input, header.config.lookahead_buffer_size) || !ByteIO::read_u8(input, header.config.min_match_length) || !ByteIO::read_u32(input, header.block_count)) {
        error = "Nevalidni vstup: soubor neobsahuje celou hlavicku.";
        return false;
    }

    const std::uint8_t known_flags = FLAG_MODEL | FLAG_ADAPTIVE_SCAN;
    if ((flags & ~known_flags) != 0) {
        error = "Nevalidni hlavicka: obsahuje nezname priznaky.";
        return false;
    }

    header.use_model = (flags & FLAG_MODEL) != 0;
    header.adaptive_scan = (flags & FLAG_ADAPTIVE_SCAN) != 0;

    return validate_header(header, error);
}
