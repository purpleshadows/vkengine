// Core/Shaders/ShaderBlob.hpp
#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <chrono>

namespace Core::Shaders {

// Fill this with what your reflection step produces
struct ReflectionInfo {
    // Example:
    // struct DescriptorBinding { uint32_t set, binding; /* type, count, stages... */ };
    // std::vector<DescriptorBinding> bindings;
    // uint32_t pushConstantSize = 0;
};

struct ShaderBlob {
    std::vector<uint32_t> spirv;              // compiled code
    std::vector<std::string> dependencies;    // absolute file paths used during preprocess
    uint64_t contentHash = 0;                 // hash(preprocessed text + stage + entry + options)
    std::chrono::file_clock::time_point newestTimestamp{}; // newest mtime among deps
    ReflectionInfo reflect;                   // extracted metadata

    // Optional helpers (not required—handy for disk cache)
    bool empty() const noexcept { return spirv.empty(); }
};

} // namespace Core::Shaders
