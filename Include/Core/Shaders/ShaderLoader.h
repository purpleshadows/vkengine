#pragma once
#include <Core/Device.h>
#include <Core/Shaders/ShaderBlob.h>
#include <Core/Shaders/ShaderCommon.h>
#include <Core/Shaders/ShaderHandle.h>
#include <Core/Shaders/ShaderKey.h>


namespace Core::Shaders {
class ShaderLoader {
public:
    explicit ShaderLoader(Device &device) : device_(device) {}
    ShaderHandle get(const ShaderKey& key);
    void pollAndReload(); // (optional; not shown here)

private:
    Device &device_;

    // contentHash -> blob (device-agnostic)
    std::unordered_map<uint64_t, std::shared_ptr<ShaderBlob>> blobCache_;

    // (blobHash, device) -> module (device-specific)
    std::unordered_map<uint64_t, std::shared_ptr<ShaderModule>> moduleCache_;

    // ShaderKey -> ShaderHandle
    std::unordered_map<ShaderKey, ShaderHandle, ShaderKeyHasher> liveHandles_;

};
}
