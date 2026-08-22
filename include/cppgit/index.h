#pragma once
#include "cppgit/repository.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace cppgit {
struct IndexEntry {
    std::uint32_t ctime_s{}, ctime_n{}, mtime_s{}, mtime_n{};
    std::uint32_t dev{}, ino{}, mode{}, uid{}, gid{}, file_size{};
    std::string oid;
    std::string path;
};

class Index {
public:
    std::vector<IndexEntry> entries;
    static Index load(const Repository& repo);
    void save(const Repository& repo) const;
    void add_path(const Repository& repo, const std::filesystem::path& path);
    void add_all(const Repository& repo);
};
}
