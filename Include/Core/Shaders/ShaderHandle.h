#pragma once
#include <memory>
#include <Core/Shaders/ShaderKey.h>
#include <Core/Shaders/ShaderModule.h>

namespace Core::Shaders {

    struct ShaderHandle {
        std::shared_ptr<const ShaderModule> module;
        ShaderKey key;
        uint64_t  version = 0;
        bool operator==(const ShaderHandle&) const = default;

        ShaderHandle(std::shared_ptr<const ShaderModule> m,
            ShaderKey k,
            uint64_t v = 0)
            : module(std::move(m)), key(std::move(k)), version(v) {}
    };
}
