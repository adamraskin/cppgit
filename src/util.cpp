#include "cppgit/util.h"
#include <openssl/sha.h>
#include <zlib.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <ctime>

namespace fs = std::filesystem;
namespace cppgit {
std::vector<std::uint8_t> read_binary(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open file: " + path.string());
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
std::string read_text(const fs::path& path) {
    auto b = read_binary(path); return std::string(b.begin(), b.end());
}
void write_binary(const fs::path& path, std::span<const std::uint8_t> data) {
    if (path.has_parent_path()) fs::create_directories(path.parent_path());
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("cannot write file: " + path.string());
    f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}
void write_text(const fs::path& path, std::string_view text) {
    if (path.has_parent_path()) fs::create_directories(path.parent_path());
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("cannot write file: " + path.string());
    f << text;
}
std::vector<std::uint8_t> sha1_raw(std::span<const std::uint8_t> data) {
    std::vector<std::uint8_t> out(SHA_DIGEST_LENGTH);
    SHA1(data.data(), data.size(), out.data()); return out;
}
std::string bytes_to_hex(std::span<const std::uint8_t> bytes) {
    static constexpr char h[]="0123456789abcdef"; std::string s; s.reserve(bytes.size()*2);
    for(auto b:bytes){s.push_back(h[b>>4]);s.push_back(h[b&15]);} return s;
}
std::string sha1_hex(std::span<const std::uint8_t> data){return bytes_to_hex(sha1_raw(data));}
std::vector<std::uint8_t> hex_to_bytes(std::string_view hex){
    if(hex.size()%2) throw std::runtime_error("invalid hex");
    auto val=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;throw std::runtime_error("invalid hex");};
    std::vector<std::uint8_t> o; o.reserve(hex.size()/2); for(size_t i=0;i<hex.size();i+=2)o.push_back((val(hex[i])<<4)|val(hex[i+1])); return o;
}
std::vector<std::uint8_t> zlib_compress(std::span<const std::uint8_t> input){
    uLongf n=compressBound(input.size()); std::vector<std::uint8_t> out(n);
    int rc=compress2(out.data(),&n,input.data(),input.size(),Z_BEST_SPEED); if(rc!=Z_OK) throw std::runtime_error("zlib compress failed"); out.resize(n); return out;
}
std::vector<std::uint8_t> zlib_decompress(std::span<const std::uint8_t> input){
    z_stream zs{}; zs.next_in=const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input.data())); zs.avail_in=input.size();
    if(inflateInit(&zs)!=Z_OK) throw std::runtime_error("zlib init failed");
    std::vector<std::uint8_t> out; std::uint8_t buf[8192]; int rc;
    do{zs.next_out=buf;zs.avail_out=sizeof(buf);rc=inflate(&zs,Z_NO_FLUSH); if(rc!=Z_OK&&rc!=Z_STREAM_END){inflateEnd(&zs);throw std::runtime_error("zlib inflate failed");} out.insert(out.end(),buf,buf+sizeof(buf)-zs.avail_out);}while(rc!=Z_STREAM_END);
    inflateEnd(&zs); return out;
}
std::string trim(std::string s){while(!s.empty()&&(s.back()=='\n'||s.back()=='\r'||s.back()==' '||s.back()=='\t'))s.pop_back();size_t i=0;while(i<s.size()&&std::isspace(static_cast<unsigned char>(s[i])))++i;return s.substr(i);}
std::uint32_t be32(const std::uint8_t* p){return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3];}
void append_be32(std::vector<std::uint8_t>& o,std::uint32_t v){o.push_back(v>>24);o.push_back(v>>16);o.push_back(v>>8);o.push_back(v);}
std::int64_t unix_time_now(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
std::string timezone_offset(){
    std::time_t t=std::time(nullptr); std::tm local=*std::localtime(&t), gm=*std::gmtime(&t); auto lt=std::mktime(&local), gt=std::mktime(&gm); long d=static_cast<long>(std::difftime(lt,gt)); char sign=d>=0?'+':'-';d=std::abs(d);int hours=static_cast<int>(d/3600); int minutes=static_cast<int>((d%3600)/60); char b[8]; std::snprintf(b,sizeof(b),"%c%02d%02d",sign,hours,minutes); return b;
}
}
