#include "cppgit/network.h"
#include "cppgit/index.h"
#include "cppgit/object_store.h"
#include "cppgit/pack.h"
#include "cppgit/refs.h"
#include "cppgit/tree.h"
#include "cppgit/util.h"
#include <curl/curl.h>
#include <filesystem>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace fs=std::filesystem;
namespace cppgit {
namespace {
size_t sink(char* ptr,size_t size,size_t nmemb,void* ud){auto*n=static_cast<std::vector<std::uint8_t>*>(ud);auto total=size*nmemb;n->insert(n->end(),reinterpret_cast<std::uint8_t*>(ptr),reinterpret_cast<std::uint8_t*>(ptr)+total);return total;}
std::vector<std::uint8_t> http(const std::string& url,const std::vector<std::uint8_t>* body=nullptr,const std::vector<std::string>& headers={}) {
    CURL* c=curl_easy_init(); if(!c) throw std::runtime_error("curl init failed"); std::vector<std::uint8_t> out; curl_slist* hs=nullptr;
    for(auto&h:headers) hs=curl_slist_append(hs,h.c_str());
    curl_easy_setopt(c,CURLOPT_URL,url.c_str());curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,sink);curl_easy_setopt(c,CURLOPT_WRITEDATA,&out);curl_easy_setopt(c,CURLOPT_USERAGENT,"cppgit/0.2");
    if(body){curl_easy_setopt(c,CURLOPT_POST,1L);curl_easy_setopt(c,CURLOPT_POSTFIELDS,reinterpret_cast<const char*>(body->data()));curl_easy_setopt(c,CURLOPT_POSTFIELDSIZE_LARGE,static_cast<curl_off_t>(body->size()));} if(hs)curl_easy_setopt(c,CURLOPT_HTTPHEADER,hs);
    auto rc=curl_easy_perform(c); long code=0;curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&code);if(hs)curl_slist_free_all(hs);curl_easy_cleanup(c);if(rc!=CURLE_OK)throw std::runtime_error(std::string("HTTP error: ")+curl_easy_strerror(rc));if(code<200||code>=300)throw std::runtime_error("HTTP status "+std::to_string(code));return out;
}
std::string pkt(std::string_view s){std::ostringstream o;o<<std::hex<<std::setw(4)<<std::setfill('0')<<(s.size()+4);return o.str()+std::string(s);}
std::vector<std::string> parse_pkts(std::span<const std::uint8_t> d){std::vector<std::string> out;std::size_t p=0;while(p+4<=d.size()){std::string h(reinterpret_cast<const char*>(d.data()+p),4);std::size_t n=std::stoul(h,nullptr,16);p+=4;if(n==0){out.emplace_back();continue;}if(n<4||p+n-4>d.size())throw std::runtime_error("invalid pkt-line");out.emplace_back(reinterpret_cast<const char*>(d.data()+p),n-4);p+=n-4;}return out;}
struct Advert { std::map<std::string,std::string> refs; std::string head_ref="refs/heads/master"; };
Advert advertise(const std::string& base){auto raw=http(base+"/info/refs?service=git-upload-pack");auto lines=parse_pkts(raw);Advert a;bool first_ref=true;for(auto line:lines){if(line.empty()||line.rfind("# service=",0)==0)continue;auto sp=line.find(' ');if(sp==std::string::npos)continue;auto oid=line.substr(0,sp);auto rest=line.substr(sp+1);auto nul=rest.find('\0');std::string name=nul==std::string::npos?rest:rest.substr(0,nul);while(!name.empty()&&(name.back()=='\n'||name.back()=='\r'))name.pop_back();a.refs[name]=oid;if(first_ref&&nul!=std::string::npos){auto caps=rest.substr(nul+1);auto s=caps.find("symref=HEAD:");if(s!=std::string::npos){auto e=caps.find_first_of(" \n\r",s);a.head_ref=caps.substr(s+12,e==std::string::npos?std::string::npos:e-(s+12));}}first_ref=false;}return a;}
std::vector<std::uint8_t> request_pack(const std::string& base,const std::string& oid){std::string req=pkt("want "+oid+" side-band-64k ofs-delta agent=cppgit/0.2\n")+"0000"+pkt("done\n");std::vector<std::uint8_t>b(req.begin(),req.end());auto resp=http(base+"/git-upload-pack",&b,{"Content-Type: application/x-git-upload-pack-request","Accept: application/x-git-upload-pack-result"});std::vector<std::uint8_t> pack;std::size_t p=0;while(p+4<=resp.size()){std::string h(reinterpret_cast<const char*>(resp.data()+p),4);std::size_t n=std::stoul(h,nullptr,16);p+=4;if(n==0)continue;if(n<4||p+n-4>resp.size())throw std::runtime_error("invalid upload-pack response");auto payload=std::span<const std::uint8_t>(resp.data()+p,n-4);p+=n-4;if(payload.size()>=4&&std::memcmp(payload.data(),"NAK\n",4)==0)continue;if(payload.empty())continue;if(payload[0]==1)pack.insert(pack.end(),payload.begin()+1,payload.end());else if(payload[0]==2)std::cerr.write(reinterpret_cast<const char*>(payload.data()+1),payload.size()-1);else if(payload[0]==3)throw std::runtime_error("remote: "+std::string(payload.begin()+1,payload.end()));else if(payload.size()>=4&&std::memcmp(payload.data(),"PACK",4)==0)pack.insert(pack.end(),payload.begin(),payload.end());}if(pack.empty())throw std::runtime_error("remote returned no pack data");return pack;}
std::string commit_tree_oid(const GitObject&c){std::string s(c.data.begin(),c.data.end());if(s.rfind("tree ",0)!=0)throw std::runtime_error("bad commit");return s.substr(5,40);}
void checkout_tree(const Repository&r,const std::string&oid,const fs::path&base){auto obj=read_object(r,oid);for(auto&e:parse_tree(obj)){auto path=base/e.name;if(e.mode=="40000"){fs::create_directories(path);checkout_tree(r,e.oid,path);}else{auto b=read_object(r,e.oid);write_binary(path,b.data);}}}
std::string basename_repo(std::string u){while(!u.empty()&&u.back()=='/')u.pop_back();auto p=u.find_last_of('/');auto n=p==std::string::npos?u:u.substr(p+1);if(n.size()>4&&n.substr(n.size()-4)==".git")n.resize(n.size()-4);return n.empty()?"repository":n;}
}
RemoteHead fetch_remote_pack(const Repository& repo,const std::string& url,const std::string& wanted_ref){auto a=advertise(url);std::string ref=wanted_ref=="HEAD"?a.head_ref:wanted_ref;if(!ref.starts_with("refs/"))ref="refs/heads/"+ref;auto it=a.refs.find(ref);if(it==a.refs.end()){auto h=a.refs.find("HEAD");if(wanted_ref=="HEAD"&&h!=a.refs.end())it=h;else throw std::runtime_error("remote ref not found: "+ref);}auto pack=request_pack(url,it->second);auto objects=unpack_pack(repo,pack);(void)objects;update_ref(repo,ref,it->second);return {it->second,ref};}
void clone_repository(const std::string& url,const std::string& directory){auto dest=directory.empty()?basename_repo(url):directory;if(fs::exists(dest)&&!fs::is_empty(dest))throw std::runtime_error("destination exists and is not empty");auto r=Repository::init(dest);auto h=fetch_remote_pack(r,url,"HEAD");write_text(r.gitdir/"HEAD","ref: "+h.head_ref+"\n");auto commit=read_object(r,h.oid);checkout_tree(r,commit_tree_oid(commit),r.worktree);auto idx=Index::load(r);idx.add_all(r);idx.save(r);fs::create_directories(r.gitdir/"refs/remotes/origin");write_text(r.gitdir/"config","[core]\n\trepositoryformatversion = 0\n\tfilemode = true\n\tbare = false\n[remote \"origin\"]\n\turl = "+url+"\n\tfetch = +refs/heads/*:refs/remotes/origin/*\n[branch \""+h.head_ref.substr(11)+"\"]\n\tremote = origin\n\tmerge = "+h.head_ref+"\n");std::cout<<"Cloned "<<url<<" into "<<dest<<"\n";}
}
