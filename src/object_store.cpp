#include "cppgit/object_store.h"
#include "cppgit/util.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace fs=std::filesystem;
namespace cppgit {
std::string write_object(const Repository& repo,std::string_view type,std::span<const std::uint8_t> data,bool actually_write){
    std::string header=std::string(type)+" "+std::to_string(data.size())+'\0'; std::vector<std::uint8_t> raw(header.begin(),header.end());raw.insert(raw.end(),data.begin(),data.end());
    auto oid=sha1_hex(raw); if(actually_write){auto p=repo.gitdir/"objects"/oid.substr(0,2)/oid.substr(2);if(!fs::exists(p))write_binary(p,zlib_compress(raw));}return oid;
}
std::string resolve_oid(const Repository& repo,std::string_view prefix){
    if(prefix.size()==40) return std::string(prefix);
    if(prefix.size()<4) throw std::runtime_error("object prefix too short");
    auto dir=repo.gitdir/"objects"/std::string(prefix.substr(0,2)); if(!fs::exists(dir))throw std::runtime_error("unknown object");std::vector<std::string> hits;
    for(auto& e:fs::directory_iterator(dir)){auto id=std::string(prefix.substr(0,2))+e.path().filename().string();if(id.rfind(prefix,0)==0)hits.push_back(id);}if(hits.size()!=1)throw std::runtime_error(hits.empty()?"unknown object":"ambiguous object prefix");return hits[0];
}
GitObject read_object(const Repository& repo,std::string_view pfx){auto oid=resolve_oid(repo,pfx);auto raw=zlib_decompress(read_binary(repo.gitdir/"objects"/oid.substr(0,2)/oid.substr(2)));auto nul=std::find(raw.begin(),raw.end(),0);if(nul==raw.end())throw std::runtime_error("invalid object");std::string h(raw.begin(),nul);auto sp=h.find(' ');if(sp==std::string::npos)throw std::runtime_error("invalid object header");std::string type=h.substr(0,sp);size_t declared=std::stoull(h.substr(sp+1));std::vector<std::uint8_t> d(nul+1,raw.end());if(d.size()!=declared)throw std::runtime_error("object size mismatch");return {type,std::move(d)};}
}
