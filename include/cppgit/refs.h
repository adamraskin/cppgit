#pragma once
#include "cppgit/repository.h"
#include <optional>
#include <string>

namespace cppgit {
std::optional<std::string> read_ref(const Repository& repo, std::string ref);
std::string head_ref(const Repository& repo);
std::optional<std::string> head_oid(const Repository& repo);
void update_ref(const Repository& repo, const std::string& ref, const std::string& oid);
}
