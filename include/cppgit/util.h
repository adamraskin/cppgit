#pragma once
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace cppgit {
std::vector<std::uint8_t> read_binary(const std::filesystem::path& path);
std::string read_text(const std::filesystem::path& path);
void write_binary(const std::filesystem::path& path, std::span<const std::uint8_t> data);
void write_text(const std::filesystem::path& path, std::string_view text);
std::string sha1_hex(std::span<const std::uint8_t> data);
std::vector<std::uint8_t> sha1_raw(std::span<const std::uint8_t> data);
std::vector<std::uint8_t> hex_to_bytes(std::string_view hex);
std::string bytes_to_hex(std::span<const std::uint8_t> bytes);
std::vector<std::uint8_t> zlib_compress(std::span<const std::uint8_t> input);
std::vector<std::uint8_t> zlib_decompress(std::span<const std::uint8_t> input);
std::string trim(std::string s);
std::uint32_t be32(const std::uint8_t* p);
void append_be32(std::vector<std::uint8_t>& out, std::uint32_t v);
std::int64_t unix_time_now();
std::string timezone_offset();
}
