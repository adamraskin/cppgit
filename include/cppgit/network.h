#pragma once
#include "cppgit/repository.h"
#include <string>
namespace cppgit {
struct RemoteHead { std::string oid; std::string head_ref; };
RemoteHead fetch_remote_pack(const Repository& repo, const std::string& url, const std::string& wanted_ref = "HEAD");
void clone_repository(const std::string& url, const std::string& directory);
}
