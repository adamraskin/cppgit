#include "cppgit/object_store.h"
#include "cppgit/repository.h"
#include "cppgit/index.h"
#include "cppgit/tree.h"
#include "cppgit/util.h"
#include <cassert>
#include <filesystem>
#include <iostream>

namespace fs=std::filesystem; using namespace cppgit;
int main(){
    auto root=fs::temp_directory_path()/"cppgit-tests"; fs::remove_all(root); auto repo=Repository::init(root);
    std::string hello="hello\n"; std::vector<std::uint8_t>d(hello.begin(),hello.end());
    auto oid=write_object(repo,"blob",d,true); assert(oid=="ce013625030ba8dba906f756967f9e9ca394464a");
    auto obj=read_object(repo,oid); assert(obj.type=="blob"); assert(std::string(obj.data.begin(),obj.data.end())==hello);
    write_text(root/"hello.txt",hello); Index idx; idx.add_path(repo,root/"hello.txt"); idx.save(repo); auto loaded=Index::load(repo); assert(loaded.entries.size()==1); assert(loaded.entries[0].path=="hello.txt"); assert(loaded.entries[0].oid==oid);
    auto tree=write_tree_from_index(repo,loaded); auto to=read_object(repo,tree); assert(to.type=="tree"); auto ents=parse_tree(to); assert(ents.size()==1&&ents[0].name=="hello.txt"&&ents[0].oid==oid);
    fs::remove_all(root); std::cout<<"all tests passed\n";
}
