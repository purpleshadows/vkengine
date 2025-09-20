// shader_key.hpp
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <initializer_list>
#include <algorithm>
#include <cstdint>
#include <Core/Shaders/ShaderCommon.h>
#include <Core/Utils/Hash/Hash.h>

namespace Core::Shaders {

struct ShaderKey {
    std::string canonicalPath;     // normalized absolute path
    Stage       stage{};           // your own stage enum (map to Vk later)
    std::string entry;             // e.g. "main"
    uint64_t    optionsHash = 0;   // computed internally

    // 1) If you already have a precomputed hash
    ShaderKey(std::string path, Stage st, std::string entryName, uint64_t prehashed)
      : canonicalPath(std::move(path)), stage(st), entry(std::move(entryName)), optionsHash(prehashed) {}

    // 2) Pass raw defines; hash inside (initializer_list is perfect for brace lists)
    ShaderKey(std::string path, Stage  st, std::string entryName,
              std::initializer_list<std::string_view> defines)
      : canonicalPath(std::move(path)), stage(st), entry(std::move(entryName)),
        optionsHash(makeOptionsHash(defines)) {}

    // 3) Vector overload if you build the list elsewhere
    ShaderKey(std::string path, Stage  st, std::string entryName,
              const std::vector<std::string>& defines)
      : canonicalPath(std::move(path)), stage(st), entry(std::move(entryName)),
        optionsHash(makeOptionsHash(defines)) {}

    bool operator==(const ShaderKey&) const = default;

private:
    // Deterministic hashing: sort to ignore define order; join with '\n'
    static uint64_t makeOptionsHash(std::initializer_list<std::string_view> defines) {
        std::vector<std::string_view> tmp(defines.begin(), defines.end());
        std::sort(tmp.begin(), tmp.end());
        std::string joined;
        joined.reserve(tmp.size() * 16);
        for (auto d : tmp) { joined.append(d.data(), d.size()); joined.push_back('\n'); }
        return Hash::fnv1a(joined);
    }

    static uint64_t makeOptionsHash(const std::vector<std::string>& defines) {
        std::vector<std::string> tmp(defines);
        std::sort(tmp.begin(), tmp.end());
        std::string joined;
        joined.reserve(tmp.size() * 16);
        for (auto& d : tmp) { joined += d; joined.push_back('\n'); }
        return Hash::fnv1a(joined);
    }
};

struct ShaderKeyHasher {
    std::size_t operator()(ShaderKey const& k) const noexcept {
        std::size_t h = 0;
        Hash::combine(h, std::hash<std::string>{}(k.canonicalPath));
        Hash::combine(h, std::hash<std::string>{}(k.entry));
        Hash::combine(h, std::hash<uint8_t>{}(static_cast<uint8_t>(k.stage)));
        Hash::combine(h, std::hash<uint64_t>{}(k.optionsHash));
        return h;
    }
};


} 
