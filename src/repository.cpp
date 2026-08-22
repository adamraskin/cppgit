#include "cppgit/repository.h"
#include "cppgit/util.h"
#include <stdexcept>
namespace fs=std::filesystem;
namespace cppgit {
Repository Repository::init(const fs::path& p){
    fs::path wt=fs::absolute(p); fs::create_directories(wt); fs::path gd=wt/".git";
    if(fs::exists(gd)) throw std::runtime_error("repository already exists");
    fs::create_directories(gd/"objects");fs::create_directories(gd/"refs"/"heads");fs::create_directories(gd/"refs"/"tags");
    write_text(gd/"HEAD","ref: refs/heads/main\n");
    write_text(gd/"config","[core]\n\trepositoryformatversion = 0\n\tfilemode = true\n\tbare = false\n");
    write_text(gd/"description","Unnamed repository; edit this file to name the repository.\n");
    return {wt,gd};
}
Repository Repository::discover(const fs::path& start){auto p=fs::absolute(start);while(true){if(fs::is_directory(p/".git"))return {p,p/".git"};auto par=p.parent_path();if(par==p)break;p=par;}throw std::runtime_error("not a cppgit/git repository");}
}
