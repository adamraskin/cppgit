#include "cppgit/index.h"
#include "cppgit/object_store.h"
#include "cppgit/util.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>
#ifndef _WIN32
#include <sys/stat.h>
#endif
namespace fs=std::filesystem;
namespace cppgit {
static IndexEntry make_entry(const Repository& repo,const fs::path& rel){
    auto full=repo.worktree/rel; auto data=read_binary(full); IndexEntry e; e.oid=write_object(repo,"blob",data,true); e.path=rel.generic_string();e.file_size=static_cast<std::uint32_t>(data.size());e.mode=0100644;
    auto ft=fs::last_write_time(full);auto sys=std::chrono::time_point_cast<std::chrono::seconds>(ft-fs::file_time_type::clock::now()+std::chrono::system_clock::now());e.mtime_s=e.ctime_s=static_cast<std::uint32_t>(sys.time_since_epoch().count());
#ifndef _WIN32
    struct stat st{}; if(::stat(full.c_str(),&st)==0){e.ctime_s=st.st_ctim.tv_sec;e.ctime_n=st.st_ctim.tv_nsec;e.mtime_s=st.st_mtim.tv_sec;e.mtime_n=st.st_mtim.tv_nsec;e.dev=st.st_dev;e.ino=st.st_ino;e.uid=st.st_uid;e.gid=st.st_gid;e.mode=st.st_mode;e.file_size=st.st_size;}
#endif
    return e;
}
Index Index::load(const Repository& repo){Index idx;auto p=repo.gitdir/"index";if(!fs::exists(p))return idx;auto b=read_binary(p);if(b.size()<12+20||std::memcmp(b.data(),"DIRC",4)!=0)throw std::runtime_error("invalid index");if(be32(b.data()+4)!=2)throw std::runtime_error("only index v2 supported");auto cnt=be32(b.data()+8);size_t off=12;for(std::uint32_t n=0;n<cnt;++n){size_t start=off;if(off+62>b.size())throw std::runtime_error("truncated index");IndexEntry e;e.ctime_s=be32(&b[off]);off+=4;e.ctime_n=be32(&b[off]);off+=4;e.mtime_s=be32(&b[off]);off+=4;e.mtime_n=be32(&b[off]);off+=4;e.dev=be32(&b[off]);off+=4;e.ino=be32(&b[off]);off+=4;e.mode=be32(&b[off]);off+=4;e.uid=be32(&b[off]);off+=4;e.gid=be32(&b[off]);off+=4;e.file_size=be32(&b[off]);off+=4;e.oid=bytes_to_hex(std::span<const std::uint8_t>(&b[off],20));off+=20;auto flags=(b[off]<<8)|b[off+1];off+=2;size_t end=off;while(end<b.size()&&b[end]!=0)++end;e.path=std::string(reinterpret_cast<const char*>(&b[off]),end-off);off=end+1;while((off-start)%8)++off;(void)flags;idx.entries.push_back(std::move(e));}return idx;}
void Index::save(const Repository& repo)const{std::vector<std::uint8_t>b={'D','I','R','C'};append_be32(b,2);append_be32(b,entries.size());for(auto&e:entries){size_t start=b.size();append_be32(b,e.ctime_s);append_be32(b,e.ctime_n);append_be32(b,e.mtime_s);append_be32(b,e.mtime_n);append_be32(b,e.dev);append_be32(b,e.ino);append_be32(b,e.mode);append_be32(b,e.uid);append_be32(b,e.gid);append_be32(b,e.file_size);auto oid=hex_to_bytes(e.oid);b.insert(b.end(),oid.begin(),oid.end());std::uint16_t flags=static_cast<std::uint16_t>(std::min<size_t>(e.path.size(),0x0fff));b.push_back(flags>>8);b.push_back(flags);b.insert(b.end(),e.path.begin(),e.path.end());b.push_back(0);while((b.size()-start)%8)b.push_back(0);}auto sum=sha1_raw(b);b.insert(b.end(),sum.begin(),sum.end());write_binary(repo.gitdir/"index",b);}
void Index::add_path(const Repository& repo,const fs::path& p){auto abs=fs::absolute(p);auto rel=abs.lexically_relative(repo.worktree);if(rel.empty()||rel.string().rfind("..",0)==0)throw std::runtime_error("path outside repository");if(fs::is_directory(abs)){for(auto& de:fs::recursive_directory_iterator(abs)){if(de.path().string().find((repo.gitdir).string())==0)continue;if(de.is_regular_file())add_path(repo,de.path());}return;}auto e=make_entry(repo,rel);auto it=std::find_if(entries.begin(),entries.end(),[&](auto&x){return x.path==e.path;});if(it==entries.end())entries.push_back(std::move(e));else *it=std::move(e);std::sort(entries.begin(),entries.end(),[](auto&a,auto&b){return a.path<b.path;});}
void Index::add_all(const Repository& repo){
    auto it=fs::recursive_directory_iterator(repo.worktree), end=fs::recursive_directory_iterator();
    for(;it!=end;++it){
        const auto& de=*it;
        if(de.path()==repo.gitdir){ if(de.is_directory()) it.disable_recursion_pending(); continue; }
        if(de.path().string().find(repo.gitdir.string()+fs::path::preferred_separator)==0) continue;
        if(de.is_regular_file()) add_path(repo,de.path());
    }
}
}
