#pragma once
#include <filesystem>

namespace cppgit {
struct Repository {
    std::filesystem::path worktree;
    std::filesystem::path gitdir;

    static Repository init(const std::filesystem::path& path);
    static Repository discover(const std::filesystem::path& start = std::filesystem::current_path());
};
}
