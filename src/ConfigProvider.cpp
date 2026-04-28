/**
 * @file ConfigProvider.cpp
 * @date 2026-04-28
 * @brief Implements default configuration and safety checks for compression.
 * @author Dominik Honza xhonza04
 */

#include "ConfigProvider.hpp"

Config ConfigProvider::get_default() {
    return Config{};
}

bool ConfigProvider::validate(const Config& config, std::string& error) {
    // A zero block would make adaptive scan grid construction impossible.
    if (config.block_dimension == 0) {
        error = "Nevalidni konfigurace: block_dimension musi byt vetsi nez 0.";
        return false;
    }

    // The assignment allows fixed blocks up to 256x256 pixels.
    if (config.block_dimension > 256) {
        error = "Nevalidni konfigurace: block_dimension muze byt maximalne 256.";
        return false;
    }

    // LZSS cannot search for references without a non-empty history window.
    if (config.history_buffer_size == 0) {
        error = "Nevalidni konfigurace: history_buffer_size musi byt vetsi nez 0.";
        return false;
    }

    // The encoder needs at least one byte of lookahead to emit literals/matches.
    if (config.lookahead_buffer_size == 0) {
        error = "Nevalidni konfigurace: lookahead_buffer_size musi byt vetsi nez 0.";
        return false;
    }

    // Shorter references would not pay for their offset/length metadata.
    if (config.min_match_length < 3) {
        error = "Nevalidni konfigurace: min_match_length musi byt alespon 3.";
        return false;
    }

    if (config.min_match_length > config.lookahead_buffer_size) {
        error = "Nevalidni konfigurace: min_match_length nesmi byt vetsi nez lookahead_buffer_size.";
        return false;
    }

    return true;
}

bool ConfigProvider::validate_for_compression(const Config& config, int image_width, std::uint64_t input_size, bool adaptive_scan, std::string& error) {
    if (!validate(config, error)) {
        return false;
    }

    // Width is required because RAW files do not store image dimensions.
    if (image_width < 1) {
        error = "Nevalidni vstup: sirka obrazu musi byt alespon 1.";
        return false;
    }

    // Empty input cannot produce a meaningful compressed image stream.
    if (input_size == 0) {
        error = "Nevalidni vstup: vstupni RAW soubor je prazdny.";
        return false;
    }

    const auto width = static_cast<std::uint64_t>(image_width);

    if (input_size % width != 0) {
        error = "Nevalidni vstup: velikost souboru neni delitelna sirkou obrazu.";
        return false;
    }

    const std::uint64_t height = input_size / width;

    // The project assignment allows assuming dimensions divisible by 256.
    if (width % 256 != 0 || height % 256 != 0) {
        error = "Nevalidni vstup: sirka i vyska obrazu musi byt nasobkem 256.";
        return false;
    }

    // Adaptive scanning splits the image into a regular grid of square blocks.
    if (adaptive_scan && (width % config.block_dimension != 0 || height % config.block_dimension != 0)) {
        error = "Nevalidni konfigurace: block_dimension musi delit sirku i vysku obrazu.";
        return false;
    }

    return true;
}
