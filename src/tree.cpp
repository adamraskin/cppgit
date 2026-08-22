#include "cppgit/tree.h"
#include "cppgit/util.h"
#include <algorithm>
#include <map>
#include <sstream>
#include <stdexcept>

namespace cppgit {
std::vector<TreeEntry> parse_tree(const GitObject& obj){
    if(obj.type!="tree") throw std::runtime_error("object is not a tree");
    std::vector<TreeEntry> out; size_t i=0;
    while(i<obj.data.size()){
        auto sp=std::find(obj.data.begin()+i,obj.data.end(),' '); if(sp==obj.data.end()) throw std::runtime_error("bad tree");
        auto nul=std::find(sp+1,obj.data.end(),0); if(nul==obj.data.end()||std::distance(nul,obj.data.end())<21) throw std::runtime_error("bad tree");
        std::string mode(obj.data.begin()+i,sp); std::string name(sp+1,nul); std::string oid=bytes_to_hex(std::span<const std::uint8_t>(&*(nul+1),20));
        out.push_back({mode,name,oid}); i=(nul-obj.data.begin())+21;
    }
    return out;
}

static std::string write_tree_node(const Repository& repo, const std::vector<IndexEntry>& entries, const std::string& prefix){
    struct Child { bool dir=false; std::string oid; };
    std::map<std::string,Child> children;
    for(const auto& e:entries){
        if(e.path.rfind(prefix,0)!=0) continue;
        auto rest=e.path.substr(prefix.size()); auto slash=rest.find('/');
        if(slash==std::string::npos) children[rest]={false,e.oid};
        else children[rest.substr(0,slash)]={true,{}};
    }
    std::vector<std::uint8_t> raw;
    for(auto& [name,c]:children){
        std::string oid=c.dir?write_tree_node(repo,entries,prefix+name+"/"):c.oid;
        std::string mode=c.dir?"40000":"100644"; std::string hdr=mode+" "+name;
        raw.insert(raw.end(),hdr.begin(),hdr.end()); raw.push_back(0); auto id=hex_to_bytes(oid); raw.insert(raw.end(),id.begin(),id.end());
    }
    return write_object(repo,"tree",raw,true);
}
std::string write_tree_from_index(const Repository& repo,const Index& index){return write_tree_node(repo,index.entries,"");}
}
