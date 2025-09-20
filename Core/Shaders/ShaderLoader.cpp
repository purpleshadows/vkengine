#include <Core/Shaders/ShaderLoader.h>
//#include <Core/Shaders/ShaderCommon.h>      // Stage, toESh(...)
#include <Core/Utils/Hash/Hash.h>               // Core::Hash::{fnv1a, combine64, ...}

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

// IMPORTANT: use the C API default limits (since DefaultTBuiltInResource was removed)
#include <glslang/Public/ResourceLimits.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>      // memcpy

namespace {

    // ---------- Private types ----------
    struct PreprocessedSource {
        std::string text;                      // fully preprocessed GLSL/HLSL
        std::vector<std::string> dependencies; // absolute paths of all #includes (+ root)
    };

    // ---------- Small utils ----------
    inline std::string readWholeFile(const std::string& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) throw std::runtime_error("Failed to open file: " + path);
        std::ostringstream oss; oss << ifs.rdbuf();
        return oss.str();
    }

    // C API fallback for resource limits (works across recent glslang versions)
    //inline const TBuiltInResource& DefaultResources() {
     //   static TBuiltInResource res = GetDefaultResources();
     //   return res;
    //}

    // ---------- Includer that tracks dependencies ----------
    class TrackingIncluder : public glslang::TShader::Includer {
    public:
        std::vector<std::string> dependencies;
        std::vector<std::filesystem::path> searchPaths; // optional extra roots

        IncludeResult* includeLocal(const char* headerName,
            const char* includerName,
            size_t) override {
            return load(headerName, includerName);
        }

        IncludeResult* includeSystem(const char* headerName,
            const char* includerName,
            size_t) override {
            return load(headerName, includerName);
        }

        void releaseInclude(IncludeResult* result) override {
            if (!result) return;
            delete[] result->headerData;
            delete result;
        }

    private:
        IncludeResult* load(const char* headerName, const char* includerName) {
            namespace fs = std::filesystem;
            fs::path includePath;

            // 1) relative to includer
            if (includerName && *includerName) {
                fs::path base = includerName;
                includePath = fs::weakly_canonical(base.parent_path() / headerName);
                if (fs::exists(includePath)) return makeResult(includePath);
            }
            // 2) search paths
            for (const auto& root : searchPaths) {
                includePath = fs::weakly_canonical(root / headerName);
                if (fs::exists(includePath)) return makeResult(includePath);
            }
            return nullptr; // glslang will emit an error
        }

        IncludeResult* makeResult(const std::filesystem::path& p) {
            std::ifstream ifs(p, std::ios::binary);
            if (!ifs) return nullptr;

            std::ostringstream oss; oss << ifs.rdbuf();
            std::string content = std::move(oss).str();
            auto* heap = new char[content.size() + 1];
            std::memcpy(heap, content.data(), content.size());
            heap[content.size()] = '\0';

            dependencies.push_back(std::filesystem::weakly_canonical(p).string());
            return new IncludeResult{ p.string(), heap, content.size(), nullptr };
        }
    };

    // If you keep a registry optionsHash -> defines, look up here.
    // For now, return empty (you can wire it later).
    inline std::vector<std::string> lookupDefines(uint64_t /*optionsHash*/) {
        return {};
    }

    // ---------- glslang preprocess ----------
    PreprocessedSource GlslangPreprocess(const std::string& sourceText,
        const std::string& sourcePath,
        Core::Shaders::Stage stage,
        std::string_view entry,
        const std::vector<std::string>& defines)
    {
        using namespace Core::Shaders;

        glslang::TShader shader(toESh(stage));

        const char* src = sourceText.c_str();
        shader.setStrings(&src, 1);

        // Preamble with #defines
        std::string preamble;
        if (!defines.empty()) {
            std::vector<std::string> defs = defines;
            std::sort(defs.begin(), defs.end());
            for (auto& d : defs) {
                if (auto pos = d.find('='); pos == std::string::npos)
                    preamble += "#define " + d + "\n";
                else
                    preamble += "#define " + d.substr(0, pos) + " " + d.substr(pos + 1) + "\n";
            }
        }
        shader.setPreamble(preamble.c_str());

        shader.setEntryPoint(entry.data());
        shader.setSourceEntryPoint(entry.data());

        // GLSL → Vulkan
        const auto lang = toESh(stage);
        shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, /*version*/100);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
        shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_5);

        // For error messages
        const char* srcs[] = { sourceText.c_str() };
        const char* names[] = { sourcePath.c_str() };
        shader.setStringsWithLengthsAndNames(srcs, nullptr, names, 1);

        TrackingIncluder includer;
        // Optional extra include roots:
        // includer.searchPaths.push_back("assets/shaders/common");

        EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);

        std::string preprocessed;
        if (!shader.preprocess(GetDefaultResources(), 100, ENoProfile, false, false,
            messages, &preprocessed, includer)) {
            throw std::runtime_error(std::string("glslang preprocess error: ") + shader.getInfoLog());
        }

        // also record the root as a dependency
        includer.dependencies.push_back(std::filesystem::weakly_canonical(sourcePath).string());

        return { std::move(preprocessed), std::move(includer.dependencies) };
    }

    // ---------- glslang compile (expects preprocessed text) ----------
    std::vector<uint32_t> GlslangCompileToSpv(const std::string& preprocessedText,
        Core::Shaders::Stage stage,
        std::string_view entry)
    {
        using namespace Core::Shaders;

        glslang::TShader shader(toESh(stage));

        const char* src = preprocessedText.c_str();
        shader.setStrings(&src, 1);

        shader.setEntryPoint(entry.data());
        shader.setSourceEntryPoint(entry.data());

        const auto lang = toESh(stage);
        shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 100);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
        shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_5);

        EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);

        if (!shader.parse(GetDefaultResources(), 100, false, messages)) {
            throw std::runtime_error(std::string("glslang parse error: ") + shader.getInfoLog());
        }

        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(messages)) {
            throw std::runtime_error(std::string("glslang link error: ") + program.getInfoLog());
        }

        std::vector<uint32_t> spirv;
        glslang::SpvOptions opts;
        opts.generateDebugInfo = false;
        opts.stripDebugInfo = true;
        opts.disableOptimizer = false;
        glslang::GlslangToSpv(*program.getIntermediate(lang), spirv, &opts);
        return spirv;
    }

    // ---------- hashing helpers ----------
    static uint64_t computeContentHash(const PreprocessedSource& src,
        const Core::Shaders::ShaderKey& key)
    {
        uint64_t h = Core::Hash::fnv1a(src.text);
        h = Core::Hash::combine64(h, static_cast<uint8_t>(key.stage));
        h = Core::Hash::combine64(h, Core::Hash::fnv1a(key.entry));
        h = Core::Hash::combine64(h, key.optionsHash);
        h = Core::Hash::combine64(h, Core::Hash::fnv1a(key.canonicalPath));
        return h;
    }

    static uint64_t makeModuleKey(uint64_t blobHash, vk::raii::Device& device) {
        // 1) Get non-RAII handle (vk::Device) via operator*()
        vk::Device vkDevWrapper = *device;
        // 2) Get raw C handle (VkDevice)
        VkDevice raw = static_cast<VkDevice>(vkDevWrapper);
        // 3) Turn the dispatchable handle (pointer-like) into an integer
        const uint64_t dev64 = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(raw));
        // 4) Hash (blobHash, device) together
        struct Pair { uint64_t a, b; } p{ blobHash, dev64 };
        return Core::Hash::fnv1a(&p, sizeof(p));
    }

} // anonymous namespace
  //

Core::Shaders::ShaderHandle
Core::Shaders::ShaderLoader::get(const Core::Shaders::ShaderKey& key) {
    // Live handle?
    if (auto it = liveHandles_.find(key); it != liveHandles_.end())
        return it->second;

    // 1) Read root file + preprocess with glslang
    const std::string source = readWholeFile(key.canonicalPath);
    const auto defines = lookupDefines(key.optionsHash); // TODO: wire your real defines
    PreprocessedSource src = GlslangPreprocess(source, key.canonicalPath, key.stage, key.entry, defines);

    // 2) Hash & fetch/compile blob
    const uint64_t contentHash = computeContentHash(src, key);

    std::shared_ptr<ShaderBlob> blob;
    if (auto bit = blobCache_.find(contentHash); bit != blobCache_.end()) {
        blob = bit->second;
    }
    else {
        std::vector<uint32_t> spirv = GlslangCompileToSpv(src.text, key.stage, key.entry);

        auto newBlob = std::make_shared<ShaderBlob>();
        newBlob->spirv = std::move(spirv);
        newBlob->dependencies = std::move(src.dependencies);
        newBlob->contentHash = contentHash;
        // newBlob->newestTimestamp = newestTimestamp(newBlob->dependencies); // implement or stub
         //newBlob->reflect         = reflect(newBlob->spirv);                // implement or stub

        blobCache_.emplace(contentHash, newBlob);
        blob = std::move(newBlob);
    }

    // 3) Build/find module (per-device)
    std::shared_ptr<ShaderModule> module;
    const uint64_t moduleKey = makeModuleKey(blob->contentHash, device_.vkDevice());

    if (auto mit = moduleCache_.find(moduleKey); mit != moduleCache_.end()) {
        module = mit->second;
    }
    else {
        vk::ShaderModuleCreateInfo ci{};
        ci.codeSize = blob->spirv.size() * sizeof(uint32_t);
        ci.pCode = blob->spirv.data();

        // If you want to keep it in your own struct:
        module = std::make_shared<ShaderModule>(device_, ci, blob->contentHash);
    }

    // 4) Handle
    ShaderHandle handle{ std::move(module), key, 1 };
    auto [it, _] = liveHandles_.emplace(key, handle);
    return it->second;
}
