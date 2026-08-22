#pragma once
#include "cppgit/index.h"
#include "cppgit/object_store.h"
#include <string>
#include <vector>

namespace cppgit {
struct TreeEntry {
    std::string mode;
    std::string name;
    std::string oid;
};

std::vector<TreeEntry> parse_tree(const GitObject& obj);
std::string write_tree_from_index(const Repository& repo, const Index& index);
}
