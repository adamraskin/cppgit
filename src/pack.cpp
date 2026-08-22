#include "cppgit/pack.h"
#include "cppgit/object_store.h"
#include "cppgit/util.h"
#include <zlib.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace cppgit {
namespace {
struct Inflated { std::vector<std::uint8_t> data; std::size_t consumed; };
Inflated inflate_one(std::span<const std::uint8_t> in) {
    z_stream zs{};
    if (inflateInit(&zs) != Z_OK) throw std::runtime_error("inflateInit failed");
    zs.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(in.data()));
    zs.avail_in = static_cast<uInt>(in.size());
    std::vector<std::uint8_t> out;
    std::uint8_t buf[32768];
    int rc;
    do {
        zs.next_out = buf; zs.avail_out = sizeof(buf);
        rc = inflate(&zs, Z_NO_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END) { inflateEnd(&zs); throw std::runtime_error("invalid zlib stream in pack"); }
        out.insert(out.end(), buf, buf + (sizeof(buf) - zs.avail_out));
    } while (rc != Z_STREAM_END);
    auto consumed = static_cast<std::size_t>(zs.total_in);
    inflateEnd(&zs);
    return {std::move(out), consumed};
}
std::uint64_t read_varint(std::span<const std::uint8_t> d, std::size_t& p) {
    std::uint64_t n=0; int shift=0;
    while (true) { if(p>=d.size()) throw std::runtime_error("truncated delta varint"); auto b=d[p++]; n |= std::uint64_t(b&0x7f)<<shift; if(!(b&0x80)) return n; shift+=7; }
}
std::vector<std::uint8_t> apply_delta(std::span<const std::uint8_t> base, std::span<const std::uint8_t> delta) {
    std::size_t p=0; auto src=read_varint(delta,p); auto dst=read_varint(delta,p);
    if(src!=base.size()) throw std::runtime_error("delta base size mismatch");
    std::vector<std::uint8_t> out; out.reserve(static_cast<std::size_t>(dst));
    while(p<delta.size()) {
        auto op=delta[p++];
        if(op&0x80) {
            std::uint32_t off=0, size=0;
            if(op&0x01) off|=delta[p++];
            if(op&0x02) off|=std::uint32_t(delta[p++])<<8;
            if(op&0x04) off|=std::uint32_t(delta[p++])<<16;
            if(op&0x08) off|=std::uint32_t(delta[p++])<<24;
            if(op&0x10) size|=delta[p++];
            if(op&0x20) size|=std::uint32_t(delta[p++])<<8;
            if(op&0x40) size|=std::uint32_t(delta[p++])<<16;
            if(size==0) size=0x10000;
            if(std::uint64_t(off)+size>base.size()) throw std::runtime_error("delta copy out of range");
            out.insert(out.end(), base.begin()+off, base.begin()+off+size);
        } else {
            if(op==0 || p+op>delta.size()) throw std::runtime_error("invalid delta insert");
            out.insert(out.end(), delta.begin()+p, delta.begin()+p+op); p+=op;
        }
    }
    if(out.size()!=dst) throw std::runtime_error("delta target size mismatch");
    return out;
}
struct Pending { std::size_t offset; int type; std::uint64_t declared; std::size_t base_offset{}; std::string base_oid; std::vector<std::uint8_t> payload; };
}
std::vector<PackedObject> unpack_pack(const Repository& repo, std::span<const std::uint8_t> p) {
    if(p.size()<32 || std::memcmp(p.data(),"PACK",4)!=0) throw std::runtime_error("invalid pack header");
    auto version=be32(p.data()+4), count=be32(p.data()+8); if(version!=2 && version!=3) throw std::runtime_error("unsupported pack version");
    auto expected=sha1_raw(p.first(p.size()-20)); if(!std::equal(expected.begin(),expected.end(),p.end()-20)) throw std::runtime_error("pack checksum mismatch");
    std::size_t pos=12; std::vector<Pending> pending; pending.reserve(count);
    for(std::uint32_t i=0;i<count;++i) {
        std::size_t start=pos; if(pos>=p.size()-20) throw std::runtime_error("truncated pack");
        auto c=p[pos++]; int type=(c>>4)&7; std::uint64_t size=c&0x0f; int shift=4;
        while(c&0x80){ if(pos>=p.size()-20) throw std::runtime_error("truncated object header"); c=p[pos++]; size|=std::uint64_t(c&0x7f)<<shift; shift+=7; }
        Pending x{}; x.offset=start; x.type=type; x.declared=size;
        if(type==6) {
            auto b=p[pos++]; std::uint64_t dist=b&0x7f;
            while(b&0x80){ b=p[pos++]; dist=((dist+1)<<7)|(b&0x7f); }
            if(dist>start) throw std::runtime_error("invalid OFS_DELTA base");
            x.base_offset=start-static_cast<std::size_t>(dist);
        } else if(type==7) {
            if(pos+20>p.size()-20) throw std::runtime_error("truncated REF_DELTA");
            x.base_oid=bytes_to_hex(p.subspan(pos,20));
            pos+=20;
        } else if(type<1 || type>4) throw std::runtime_error("unsupported packed object type");
        auto z=inflate_one(p.subspan(pos,p.size()-20-pos)); pos+=z.consumed; x.payload=std::move(z.data); pending.push_back(std::move(x));
    }
    std::unordered_map<std::size_t,PackedObject> by_offset; std::unordered_map<std::string,PackedObject> by_oid; std::vector<PackedObject> result; result.reserve(count);
    std::size_t resolved=0;
    for(std::size_t pass=0; resolved<count && pass<count+1; ++pass) {
        bool progress=false;
        for(auto& x:pending) {
            if(by_offset.contains(x.offset)) continue;
            PackedObject obj;
            if(x.type>=1 && x.type<=4) {
                static const char* names[]={"","commit","tree","blob","tag"}; obj.type=names[x.type]; obj.data=x.payload;
                if(obj.data.size()!=x.declared) throw std::runtime_error("packed object size mismatch");
            } else {
                const PackedObject* base=nullptr;
                if(x.type==6) { auto it=by_offset.find(x.base_offset); if(it!=by_offset.end()) base=&it->second; }
                else { auto it=by_oid.find(x.base_oid); if(it!=by_oid.end()) base=&it->second; else { try { auto ro=read_object(repo,x.base_oid); static PackedObject tmp; tmp={ro.type,ro.data,x.base_oid}; base=&tmp; } catch(...){} } }
                if(!base) continue;
                if(x.payload.size()!=x.declared) throw std::runtime_error("delta payload size mismatch");
                obj.type=base->type; obj.data=apply_delta(base->data,x.payload);
            }
            obj.oid=write_object(repo,obj.type,obj.data,true); by_offset[x.offset]=obj; by_oid[obj.oid]=obj; result.push_back(obj); ++resolved; progress=true;
        }
        if(!progress && resolved<count) throw std::runtime_error("could not resolve pack deltas");
    }
    return result;
}
}
