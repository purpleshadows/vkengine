// Core/Shaders/ShaderModule.hpp
#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

//check home
namespace Core::Shaders {

struct ShaderModule {
    VkShaderModule vkModule = VK_NULL_HANDLE;
    VkDevice       device   = VK_NULL_HANDLE;  // not owned
    uint64_t       blobHash = 0;               // link to ShaderBlob's contentHash

    ShaderModule() = default;
    ShaderModule(VkDevice dev, VkShaderModule mod, uint64_t hash)
        : vkModule(mod), device(dev), blobHash(hash) {}

    ShaderModule(const ShaderModule&)            = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;

    ShaderModule(ShaderModule&& other) noexcept
        : vkModule(other.vkModule), device(other.device), blobHash(other.blobHash) {
        other.vkModule = VK_NULL_HANDLE; other.device = VK_NULL_HANDLE; other.blobHash = 0;
    }
    ShaderModule& operator=(ShaderModule&& other) noexcept {
        if (this != &other) {
            destroy();
            vkModule = other.vkModule; device = other.device; blobHash = other.blobHash;
            other.vkModule = VK_NULL_HANDLE; other.device = VK_NULL_HANDLE; other.blobHash = 0;
        }
        return *this;
    }

    ~ShaderModule() { destroy(); }

private:
    void destroy() noexcept {
        if (vkModule != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, vkModule, nullptr);
            vkModule = VK_NULL_HANDLE;
        }
    }
};

} // namespace Core::Shaders
