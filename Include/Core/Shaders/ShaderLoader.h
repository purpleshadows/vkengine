#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
namespace Core::Shaders {
class ShaderLoader {
    public:
    const std::vector<uint32_t> &loadSpirv(const std::filesystem::path &filepath);

    // clear the cache
    void clearCache();
    // Explicitly reload a specific shader (invalidate cache entry)
    void reload(const std::filesystem::path &filepath);

    private:
    struct CacheEntry {
        std::vector<uint32_t> data;
        std::filesystem::file_time_type lastWriteTime;
    };

    // key = file path (string), value = cached SPIR-V data + timestamp
    std::unordered_map<std::string, CacheEntry> cache_;
};
}
