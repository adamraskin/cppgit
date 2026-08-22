#include "cppgit/diff.h"
#include "cppgit/index.h"
#include "cppgit/network.h"
#include "cppgit/pack.h"
#include "cppgit/object_store.h"
#include "cppgit/refs.h"
#include "cppgit/repository.h"
#include "cppgit/tree.h"
#include "cppgit/util.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace fs=std::filesystem; using namespace cppgit;
static void usage(){std::cerr<<"cppgit <init|clone|fetch|unpack-pack|hash-object|cat-file|add|write-tree|ls-tree|commit-tree|commit|log|branch|checkout|status|diff> ...\n";}
static std::string ident(){const char* n=std::getenv("GIT_AUTHOR_NAME");const char* e=std::getenv("GIT_AUTHOR_EMAIL");return std::string(n?n:"CppGit User")+" <"+(e?e:"cppgit@example.com")+">";}
static std::string make_commit(const Repository&r,const std::string&tree,const std::string&msg,const std::optional<std::string>&parent){std::ostringstream s;s<<"tree "<<tree<<"\n";if(parent)s<<"parent "<<*parent<<"\n";auto stamp=std::to_string(unix_time_now())+" "+timezone_offset();s<<"author "<<ident()<<" "<<stamp<<"\ncommitter "<<ident()<<" "<<stamp<<"\n\n"<<msg;if(msg.empty()||msg.back()!='\n')s<<'\n';auto str=s.str();return write_object(r,"commit",std::span<const std::uint8_t>((const std::uint8_t*)str.data(),str.size()),true);}
static std::string commit_tree_oid(const GitObject&c){std::string s(c.data.begin(),c.data.end());auto p=s.find("tree ");if(p!=0)throw std::runtime_error("bad commit");return s.substr(5,40);}
static void checkout_tree(const Repository&r,const std::string&oid,const fs::path&base){auto obj=read_object(r,oid);for(auto&e:parse_tree(obj)){auto p=base/e.name;if(e.mode=="40000"){fs::create_directories(p);checkout_tree(r,e.oid,p);}else{auto b=read_object(r,e.oid);write_binary(p,b.data);}}}
int main(int argc,char**argv){try{if(argc<2){usage();return 1;}std::string cmd=argv[1];
    if(cmd=="init"){auto p=argc>2?fs::path(argv[2]):fs::current_path();auto r=Repository::init(p);std::cout<<"Initialized empty Git-compatible repository in "<<r.gitdir.string()<<"\n";return 0;}
    if(cmd=="clone"){if(argc<3)throw std::runtime_error("clone <url> [directory]");clone_repository(argv[2],argc>3?argv[3]:"");return 0;}
    auto r=Repository::discover();
    if(cmd=="fetch"){if(argc<3)throw std::runtime_error("fetch <url> [ref]");auto h=fetch_remote_pack(r,argv[2],argc>3?argv[3]:"HEAD");std::cout<<"Fetched "<<h.head_ref<<" -> "<<h.oid<<"\n";return 0;}
    if(cmd=="unpack-pack"){if(argc<3)throw std::runtime_error("unpack-pack <pack-file>");auto d=read_binary(argv[2]);auto objects=unpack_pack(r,d);std::cout<<"Unpacked "<<objects.size()<<" objects\n";return 0;}
    if(cmd=="hash-object"){bool w=false;int i=2;if(i<argc&&std::string(argv[i])=="-w"){w=true;++i;}if(i>=argc)throw std::runtime_error("hash-object requires file");auto d=read_binary(argv[i]);std::cout<<write_object(r,"blob",d,w)<<"\n";}
    else if(cmd=="cat-file"){if(argc<4)throw std::runtime_error("cat-file -p|-t|-s <object>");auto o=read_object(r,argv[3]);std::string opt=argv[2];if(opt=="-t")std::cout<<o.type<<"\n";else if(opt=="-s")std::cout<<o.data.size()<<"\n";else if(opt=="-p"){if(o.type=="tree"){for(auto&e:parse_tree(o))std::cout<<e.mode<<" "<<(e.mode=="40000"?"tree":"blob")<<" "<<e.oid<<"\t"<<e.name<<"\n";}else std::cout.write((char*)o.data.data(),o.data.size());}else throw std::runtime_error("unknown cat-file option");}
    else if(cmd=="add"){if(argc<3)throw std::runtime_error("add requires paths");auto idx=Index::load(r);for(int i=2;i<argc;++i){std::string p=argv[i];if(p==".")idx.add_all(r);else idx.add_path(r,p);}idx.save(r);}
    else if(cmd=="write-tree"){auto idx=Index::load(r);std::cout<<write_tree_from_index(r,idx)<<"\n";}
    else if(cmd=="ls-tree"){if(argc<3)throw std::runtime_error("ls-tree requires tree");auto o=read_object(r,argv[2]);for(auto&e:parse_tree(o))std::cout<<e.mode<<" "<<(e.mode=="40000"?"tree":"blob")<<" "<<e.oid<<"\t"<<e.name<<"\n";}
    else if(cmd=="commit-tree"){if(argc<5||std::string(argv[3])!="-m")throw std::runtime_error("commit-tree <tree> -m <message> [-p parent]");std::optional<std::string> p;for(int i=5;i+1<argc;++i)if(std::string(argv[i])=="-p")p=argv[i+1];std::cout<<make_commit(r,argv[2],argv[4],p)<<"\n";}
    else if(cmd=="commit"){if(argc<4||std::string(argv[2])!="-m")throw std::runtime_error("commit -m <message>");auto idx=Index::load(r);auto tree=write_tree_from_index(r,idx);auto oid=make_commit(r,tree,argv[3],head_oid(r));update_ref(r,head_ref(r),oid);std::cout<<"["<<head_ref(r).substr(11)<<" "<<oid.substr(0,7)<<"] "<<argv[3]<<"\n";}
    else if(cmd=="log"){auto cur=head_oid(r);while(cur){auto o=read_object(r,*cur);std::string s(o.data.begin(),o.data.end());std::cout<<"commit "<<*cur<<"\n";auto a=s.find("author "),blank=s.find("\n\n");if(a!=std::string::npos){auto e=s.find('\n',a);std::cout<<"Author: "<<s.substr(a+7,e-(a+7))<<"\n";}if(blank!=std::string::npos)std::cout<<"\n    "<<trim(s.substr(blank+2))<<"\n\n";auto pp=s.find("parent ");if(pp==std::string::npos)cur.reset();else cur=s.substr(pp+7,40);}}
    else if(cmd=="branch"){if(argc==2){auto cur=head_ref(r);for(auto&de:fs::directory_iterator(r.gitdir/"refs/heads")){auto n=de.path().filename().string();std::cout<<(cur=="refs/heads/"+n?"* ":"  ")<<n<<"\n";}}else{auto h=head_oid(r);if(!h)throw std::runtime_error("cannot branch before first commit");update_ref(r,"refs/heads/"+std::string(argv[2]),*h);}}
    else if(cmd=="checkout"){if(argc<3)throw std::runtime_error("checkout requires branch");std::string ref="refs/heads/"+std::string(argv[2]);auto oid=read_ref(r,ref);if(!oid)throw std::runtime_error("unknown branch");auto c=read_object(r,*oid);auto tree=commit_tree_oid(c);write_text(r.gitdir/"HEAD","ref: "+ref+"\n");checkout_tree(r,tree,r.worktree);std::cout<<"Switched to branch '"<<argv[2]<<"'\n";}
    else if(cmd=="status"){std::cout<<"On branch "<<head_ref(r).substr(11)<<"\n";auto idx=Index::load(r);bool dirty=false;for(auto&e:idx.entries){auto p=r.worktree/e.path;if(!fs::exists(p)){std::cout<<"\tdeleted: "<<e.path<<"\n";dirty=true;continue;}auto d=read_binary(p);auto oid=write_object(r,"blob",d,false);if(oid!=e.oid){std::cout<<"\tmodified: "<<e.path<<"\n";dirty=true;}}if(!dirty)std::cout<<"nothing to commit, working tree clean\n";}
    else if(cmd=="diff"){auto idx=Index::load(r);for(auto&e:idx.entries){auto p=r.worktree/e.path;if(!fs::exists(p))continue;auto old=read_object(r,e.oid);auto nw=read_text(p);std::string os(old.data.begin(),old.data.end());auto d=simple_unified_diff(os,nw,e.path,e.path);if(!d.empty())std::cout<<d;}}
    else{usage();return 1;}return 0;
}catch(const std::exception&e){std::cerr<<"cppgit: "<<e.what()<<"\n";return 1;}}
