#pragma once
#include "cppgit/repository.h"
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace cppgit {
struct PackedObject { std::string type; std::vector<std::uint8_t> data; std::string oid; };
std::vector<PackedObject> unpack_pack(const Repository& repo, std::span<const std::uint8_t> pack_data);
}
