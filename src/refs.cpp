#include "cppgit/refs.h"
#include "cppgit/util.h"
#include <filesystem>
namespace cppgit {
std::optional<std::string> read_ref(const Repository& repo,std::string ref){for(int i=0;i<8;++i){auto p=repo.gitdir/ref;if(!std::filesystem::exists(p))return std::nullopt;auto v=trim(read_text(p));if(v.rfind("ref: ",0)==0){ref=v.substr(5);continue;}return v;}return std::nullopt;}
std::string head_ref(const Repository& repo){auto h=trim(read_text(repo.gitdir/"HEAD"));return h.rfind("ref: ",0)==0?h.substr(5):"HEAD";}
std::optional<std::string> head_oid(const Repository& repo){return read_ref(repo,"HEAD");}
void update_ref(const Repository& repo,const std::string& ref,const std::string& oid){write_text(repo.gitdir/ref,oid+"\n");}
}
