#include "cppgit/diff.h"
#include <sstream>
#include <vector>

static std::vector<std::string> lines(const std::string& s){std::vector<std::string> v;std::istringstream in(s);std::string x;while(std::getline(in,x))v.push_back(x);return v;}
std::string simple_unified_diff(const std::string& a,const std::string& b,const std::string& an,const std::string& bn){
    auto A=lines(a),B=lines(b); if(A==B)return{}; std::ostringstream o;o<<"--- a/"<<an<<"\n+++ b/"<<bn<<"\n@@ -1,"<<A.size()<<" +1,"<<B.size()<<" @@\n";
    size_t n=std::max(A.size(),B.size());for(size_t i=0;i<n;++i){if(i<A.size()&&i<B.size()&&A[i]==B[i])o<<' '<<A[i]<<'\n';else{if(i<A.size())o<<'-'<<A[i]<<'\n';if(i<B.size())o<<'+'<<B[i]<<'\n';}}return o.str();
}
