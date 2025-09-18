#pragma once
#include <memory>
#include "gfx/shader/ShaderKey.hpp"

namespace Core::Shaders {
struct ShaderModule; // defined elsewhere

struct ShaderHandle {
    std::shared_ptr<const ShaderModule> module;
    ShaderKey key;
    uint64_t  version = 0;
    bool operator==(const ShaderHandle&) const = default;
};
}
