/**
 * @file ByteIO.hpp
 * @date 2026-04-28
 * @brief Declares IO operation for utility use.
 * @author Dominik Honza xhonza04
 */
#pragma once

#include <cstdint>
#include <vector>
#include <istream>
#include <ostream>

/**
 * ByteIO
 * Utility class for reading/writing primitive types in little-endian format.
 */
class ByteIO {
public:

    // ================= STREAM WRITE =================

    static bool write_u8(std::ostream& out, std::uint8_t value);
    static bool write_u16(std::ostream& out, std::uint16_t value);
    static bool write_u32(std::ostream& out, std::uint32_t value);

    // ================= STREAM READ =================

    static bool read_u8(std::istream& in, std::uint8_t& value);
    static bool read_u16(std::istream& in, std::uint16_t& value);
    static bool read_u32(std::istream& in, std::uint32_t& value);

    // ================= VECTOR WRITE =================

    static void write_u8(std::vector<std::uint8_t>& out, std::uint8_t value);
    static void write_u16(std::vector<std::uint8_t>& out, std::uint16_t value);
    static void write_u32(std::vector<std::uint8_t>& out, std::uint32_t value);

    // ================= VECTOR READ =================

    static bool read_u8(const std::vector<std::uint8_t>& in, std::size_t& pos, std::uint8_t& value);
    static bool read_u16(const std::vector<std::uint8_t>& in, std::size_t& pos, std::uint16_t& value);
    static bool read_u32(const std::vector<std::uint8_t>& in, std::size_t& pos, std::uint32_t& value);

    // ================= HELPER =================

    static bool has_bytes(const std::vector<std::uint8_t>& in, std::size_t pos, std::size_t count);
};