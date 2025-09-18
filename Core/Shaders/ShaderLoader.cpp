#include <Core/Shaders/ShaderLoader.h>
#include <fstream>
#include <stdexcept>
#include <ios>

const std::vector<uint32_t> &Core::Shaders::ShaderLoader::loadSpirv(const std::filesystem::path &filepath) {
    std::string key = filepath.string();

    // Get current file last write time
    std::filesystem::file_time_type currentWriteTime;
    try {
        currentWriteTime = std::filesystem::last_write_time(filepath);
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("ShaderLoader: cannot fetch last_write_time for " + key + ": " + e.what());
    }

    auto it = cache_.find(key);
    if (it != cache_.end()) {
        // If the file hasn't changed, return the cached data
        if (it->second.lastWriteTime == currentWriteTime) {
            return it->second.data;
        }
        // else, file changed → reload
    }

    // Load file content
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("ShaderLoader: failed to open SPIR-V file: " + key);
    }

    uint64_t fileSize = static_cast<uint64_t>(file.tellg());
    if (fileSize == 0) {
        throw std::runtime_error("ShaderLoader: SPIR-V file is empty: " + key);
    }
    if (fileSize % 4 != 0) {
        throw std::runtime_error("ShaderLoader: SPIR-V file size not multiple of 4: " + key);
    }

    std::vector<uint32_t> buffer;
    buffer.resize(fileSize / 4);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    if (!file) {
        throw std::runtime_error("ShaderLoader: error reading SPIR-V file " + key);
    }
    file.close();

    // Insert/update cache
    CacheEntry entry;
    entry.data = std::move(buffer);
    entry.lastWriteTime = currentWriteTime;

    // The map stores the buffer; get its reference
    auto& inserted = cache_[key] = std::move(entry);

    return inserted.data;
}

void Core::Shaders::ShaderLoader::clearCache() {
    cache_.clear();
}

void Core::Shaders::ShaderLoader::reload(const std::filesystem::path& filepath) {
    cache_.erase(filepath.string());
}
