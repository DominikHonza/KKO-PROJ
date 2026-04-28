/**
 * @file ByteIO.cpp
 * @date 2026-04-28
 * @brief Implements shared IO operation fucntions for utility use.
 * @author Dominik Honza xhonza04
 */
#include "ByteIO.hpp"

// ================= STREAM WRITE =================

bool ByteIO::write_u8(std::ostream& out, std::uint8_t value) {
    out.put(static_cast<char>(value));
    return out.good();
}

bool ByteIO::write_u16(std::ostream& out, std::uint16_t value) {
    return write_u8(out, value & 0xFF) &&
           write_u8(out, (value >> 8) & 0xFF);
}

bool ByteIO::write_u32(std::ostream& out, std::uint32_t value) {
    return write_u8(out, value & 0xFF) &&
           write_u8(out, (value >> 8) & 0xFF) &&
           write_u8(out, (value >> 16) & 0xFF) &&
           write_u8(out, (value >> 24) & 0xFF);
}

// ================= STREAM READ =================

bool ByteIO::read_u8(std::istream& in, std::uint8_t& value) {
    char byte = 0;
    if (!in.get(byte)) return false;

    value = static_cast<std::uint8_t>(byte);
    return true;
}

bool ByteIO::read_u16(std::istream& in, std::uint16_t& value) {
    std::uint8_t b0, b1;

    if (!read_u8(in, b0) || !read_u8(in, b1)) return false;

    value = static_cast<std::uint16_t>(b0) |
           (static_cast<std::uint16_t>(b1) << 8);

    return true;
}

bool ByteIO::read_u32(std::istream& in, std::uint32_t& value) {
    std::uint8_t b0, b1, b2, b3;

    if (!read_u8(in, b0) || !read_u8(in, b1) ||
        !read_u8(in, b2) || !read_u8(in, b3)) return false;

    value = static_cast<std::uint32_t>(b0) |
           (static_cast<std::uint32_t>(b1) << 8) |
           (static_cast<std::uint32_t>(b2) << 16) |
           (static_cast<std::uint32_t>(b3) << 24);

    return true;
}

// ================= VECTOR WRITE =================

void ByteIO::write_u8(std::vector<std::uint8_t>& out, std::uint8_t value) {
    out.push_back(value);
}

void ByteIO::write_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(value & 0xFF);
    out.push_back((value >> 8) & 0xFF);
}

void ByteIO::write_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(value & 0xFF);
    out.push_back((value >> 8) & 0xFF);
    out.push_back((value >> 16) & 0xFF);
    out.push_back((value >> 24) & 0xFF);
}

// ================= VECTOR READ =================

bool ByteIO::read_u8(const std::vector<std::uint8_t>& in, std::size_t& pos, std::uint8_t& value) {
    if (pos >= in.size()) return false;

    value = in[pos++];
    return true;
}

bool ByteIO::read_u16(const std::vector<std::uint8_t>& in, std::size_t& pos, std::uint16_t& value) {
    if (pos + 1 >= in.size()) return false;

    value = static_cast<std::uint16_t>(in[pos]) |
           (static_cast<std::uint16_t>(in[pos + 1]) << 8);

    pos += 2;
    return true;
}

bool ByteIO::read_u32(const std::vector<std::uint8_t>& in, std::size_t& pos, std::uint32_t& value) {
    if (pos + 3 >= in.size()) return false;

    value = static_cast<std::uint32_t>(in[pos]) |
           (static_cast<std::uint32_t>(in[pos + 1]) << 8) |
           (static_cast<std::uint32_t>(in[pos + 2]) << 16) |
           (static_cast<std::uint32_t>(in[pos + 3]) << 24);

    pos += 4;
    return true;
}

// ================= HELPER =================

bool ByteIO::has_bytes(const std::vector<std::uint8_t>& in, std::size_t pos, std::size_t count) {
    return pos + count <= in.size();
}