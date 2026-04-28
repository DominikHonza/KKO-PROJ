/**
 * @file ByteIO.cpp
 * @date 2026-04-28
 * @brief Implements shared IO operation fucntions for utility use.
 * @author Dominik Honza xhonza04
 */
#include "ByteIO.hpp"

// ================= STREAM WRITE =================

/**
 * @brief Writes 8-bit unsigned integer to stream.
 * @param out Output stream.
 * @param value Value to write.
 * @return true on success, false otherwise.
 */
bool ByteIO::write_u8(std::ostream& out, std::uint8_t value) {
    out.put(static_cast<char>(value));
    return out.good();
}

/**
 * @brief Writes 16-bit unsigned integer to stream (little-endian).
 * @param out Output stream.
 * @param value Value to write.
 * @return true on success, false otherwise.
 */
bool ByteIO::write_u16(std::ostream& out, std::uint16_t value) {
    return write_u8(out, value & 0xFF) &&
           write_u8(out, (value >> 8) & 0xFF);
}

/**
 * @brief Writes 32-bit unsigned integer to stream (little-endian).
 * @param out Output stream.
 * @param value Value to write.
 * @return true on success, false otherwise.
 */
bool ByteIO::write_u32(std::ostream& out, std::uint32_t value) {
    return write_u8(out, value & 0xFF) &&
           write_u8(out, (value >> 8) & 0xFF) &&
           write_u8(out, (value >> 16) & 0xFF) &&
           write_u8(out, (value >> 24) & 0xFF);
}

// ================= STREAM READ =================

/**
 * @brief Reads 8-bit unsigned integer from stream.
 * @param in Input stream.
 * @param value Output value.
 * @return true on success, false otherwise.
 */
bool ByteIO::read_u8(std::istream& in, std::uint8_t& value) {
    char byte = 0;
    if (!in.get(byte)) return false;

    value = static_cast<std::uint8_t>(byte);
    return true;
}

/**
 * @brief Reads 16-bit unsigned integer from stream (little-endian).
 * @param in Input stream.
 * @param value Output value.
 * @return true on success, false otherwise.
 */
bool ByteIO::read_u16(std::istream& in, std::uint16_t& value) {
    std::uint8_t b0, b1;

    if (!read_u8(in, b0) || !read_u8(in, b1)) return false;

    value = static_cast<std::uint16_t>(b0) |
           (static_cast<std::uint16_t>(b1) << 8);

    return true;
}

/**
 * @brief Reads 32-bit unsigned integer from stream (little-endian).
 * @param in Input stream.
 * @param value Output value.
 * @return true on success, false otherwise.
 */
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

/**
 * @brief Writes 8-bit unsigned integer to vector.
 * @param out Output buffer.
 * @param value Value to write.
 */
void ByteIO::write_u8(std::vector<std::uint8_t>& out, std::uint8_t value) {
    out.push_back(value);
}

/**
 * @brief Writes 16-bit unsigned integer to vector (little-endian).
 * @param out Output buffer.
 * @param value Value to write.
 */
void ByteIO::write_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(value & 0xFF);
    out.push_back((value >> 8) & 0xFF);
}

/**
 * @brief Writes 32-bit unsigned integer to vector (little-endian).
 * @param out Output buffer.
 * @param value Value to write.
 */
void ByteIO::write_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(value & 0xFF);
    out.push_back((value >> 8) & 0xFF);
    out.push_back((value >> 16) & 0xFF);
    out.push_back((value >> 24) & 0xFF);
}

// ================= VECTOR READ =================

/**
 * @brief Reads 8-bit unsigned integer from vector.
 * @param in Input buffer.
 * @param pos Current position (updated).
 * @param value Output value.
 * @return true on success, false otherwise.
 */
bool ByteIO::read_u8(const std::vector<std::uint8_t>& in, std::size_t& pos, std::uint8_t& value) {
    if (pos >= in.size()) return false;

    value = in[pos++];
    return true;
}

/**
 * @brief Reads 16-bit unsigned integer from vector (little-endian).
 * @param in Input buffer.
 * @param pos Current position (updated).
 * @param value Output value.
 * @return true on success, false otherwise.
 */
bool ByteIO::read_u16(const std::vector<std::uint8_t>& in, std::size_t& pos, std::uint16_t& value) {
    if (pos + 1 >= in.size()) return false;

    value = static_cast<std::uint16_t>(in[pos]) |
           (static_cast<std::uint16_t>(in[pos + 1]) << 8);

    pos += 2;
    return true;
}

/**
 * @brief Reads 32-bit unsigned integer from vector (little-endian).
 * @param in Input buffer.
 * @param pos Current position (updated).
 * @param value Output value.
 * @return true on success, false otherwise.
 */
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


/**
 * @brief Checks if buffer contains enough bytes from given position.
 * @param in Input buffer.
 * @param pos Current position.
 * @param count Number of bytes required.
 * @return true if enough bytes available, false otherwise.
 */
bool ByteIO::has_bytes(const std::vector<std::uint8_t>& in, std::size_t pos, std::size_t count) {
    return pos + count <= in.size();
}