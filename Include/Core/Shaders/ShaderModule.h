// Core/Shaders/ShaderModule.hpp
#pragma once
#include <Core/Device.h>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
// check home
namespace Core::Shaders {
    struct ShaderModule {
        vk::raii::ShaderModule module = nullptr; // owns/destroys VkShaderModule
        uint64_t blobHash = 0;

        ShaderModule() = default;

        ShaderModule(Core::Device const& dev, vk::ShaderModuleCreateInfo const& ci,
            uint64_t hash)
            : module{ dev.vkDevice(), ci }, blobHash{ hash } {
        }

        // convenience when you need the raw VkShaderModule
        vk::ShaderModule raw() const noexcept { return *module; }

        // move-only (vk::raii::ShaderModule is already move-only)
        ShaderModule(ShaderModule&&) noexcept = default;
        ShaderModule& operator=(ShaderModule&&) noexcept = default;

        ShaderModule(const ShaderModule&) = delete;
        ShaderModule& operator=(const ShaderModule&) = delete;
    };
} // namespace Core::Shaders
