#pragma once
#include "cppgit/repository.h"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cppgit {
struct GitObject {
    std::string type;
    std::vector<std::uint8_t> data;
};

std::string write_object(const Repository& repo, std::string_view type,
                         std::span<const std::uint8_t> data, bool actually_write = true);
GitObject read_object(const Repository& repo, std::string_view oid_prefix);
std::string resolve_oid(const Repository& repo, std::string_view prefix);
}
