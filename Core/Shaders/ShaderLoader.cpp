#include <Core/Shaders/ShaderLoader.h>
#include <stdexcept>

namespace {
    struct PreprocessedSource {
        std::string text;                      // fully preprocessed GLSL/HLSL
        std::vector<std::string> dependencies; // absolute paths of all #includes
    };

    inline std::string readWholeFile(const std::string& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) throw std::runtime_error("Failed to open file: " + path);
        std::ostringstream oss; oss << ifs.rdbuf();
        return oss.str();
    }

    // Includer that tracks dependencies; inherits glslang helper
    struct TrackingIncluder : public glslang::TShader::Includer {
        std::vector<std::string> dependencies;

        static std::string readFile(const std::filesystem::path& p) {
            std::ifstream ifs(p, std::ios::binary);
            if (!ifs) return {};
            std::ostringstream oss; oss << ifs.rdbuf(); return oss.str();
        }

        IncludeResult* includeSystem(const char*, const char*, size_t) override {
            // Not handling <...> form; add if you use it
            return nullptr;
        }

        IncludeResult* includeLocal(const char* headerName,
                                    const char* includerName,
                                    size_t) override {
            std::filesystem::path base = includerName ? includerName : "";
            std::filesystem::path inc  = std::filesystem::weakly_canonical(base.parent_path() / headerName);
            std::string content = readFile(inc);
            if (content.empty()) return nullptr;

            dependencies.push_back(inc.string());

            char* heap = new char[content.size()+1];
            memcpy(heap, content.data(), content.size());
            heap[content.size()] = '\0';
            return new IncludeResult(inc.string(), heap, content.size(), nullptr);
        }

        void releaseInclude(IncludeResult* result) override {
            delete[] result->headerData;
            delete result;
        }
    };

    // If you have a registry mapping optionsHash -> defines list, look it up here.
    // Otherwise, just return {} for now.
    inline std::vector<std::string> lookupDefines(uint64_t /*optionsHash*/) {
        return {}; // TODO: wire to your real defines source
    }

    inline const TBuiltInResource& DefaultResources() {
        static TBuiltInResource res = glslang::DefaultTBuiltInResource;
        return res;
    }


    // prototypes for private helpers
    PreprocessedSource GlslangPreprocess(const std::string& sourceText,
                                     const std::string& sourcePath,
                                     Stage stage,
                                     std::string_view entry,
                                     const std::vector<std::string>& defines /* "NAME" or "NAME=VAL" */)
    {
        EShLanguage lang = toESh(stage);
        glslang::TShader shader(lang);

        // Set source
        const char* src = sourceText.c_str();
        shader.setStrings(&src, 1);

        // Set preamble for #defines (this is the easiest way)
        std::string preamble;
        if (!defines.empty()) {
            std::vector<std::string> defs = defines;
            std::sort(defs.begin(), defs.end());
            for (auto& d : defs) {
                auto pos = d.find('=');
                if (pos == std::string::npos) preamble += "#define " + d + "\n";
                else preamble += "#define " + d.substr(0,pos) + " " + d.substr(pos+1) + "\n";
            }
        }
        shader.setPreamble(preamble.c_str());

        // Entry + environment
        shader.setEntryPoint(entry.data());
        shader.setSourceEntryPoint(entry.data());

        shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, /*version*/ 100);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
        shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_5);

        // For error messages
        shader.setStringsWithLengthsAndNames(&src, nullptr, &sourcePath.c_str(), 1);

        FileIncluder includer; // collects dependencies
        EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);

        std::string preprocessed;
        if (!shader.preprocess(&DefaultResources(), 100, ENoProfile, false, false, messages, &preprocessed, includer)) {
            throw std::runtime_error(std::string("glslang preprocess error: ") + shader.getInfoLog());
        }

        return { std::move(preprocessed), std::move(includer.dependencies) };
    }


    std::vector<uint32_t> GlslangCompileToSpv(const std::string& preprocessedText,
                                          Stage stage,
                                          std::string_view entry)
    {
        EShLanguage lang = toESh(stage);
        glslang::TShader shader(lang);

        const char* src = preprocessedText.c_str();
        shader.setStrings(&src, 1);

        shader.setEntryPoint(entry.data());
        shader.setSourceEntryPoint(entry.data());

        shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 100);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
        shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_5);

        EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);

        // Parse (type-check, etc.)
        if (!shader.parse(&DefaultResources(), 100, false, messages)) {
            throw std::runtime_error(std::string("glslang parse error: ") + shader.getInfoLog());
        }

        // Link into a program (even single-stage)
        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(messages)) {
            throw std::runtime_error(std::string("glslang link error: ") + program.getInfoLog());
        }

        // SPIR-V generation
        std::vector<uint32_t> spirv;
        glslang::SpvOptions opts;
        opts.generateDebugInfo = false;
        opts.stripDebugInfo = true;
        opts.disableOptimizer = false;
        glslang::GlslangToSpv(*program.getIntermediate(lang), spirv, &opts);
        return spirv;
    }

    // Might need in the future
    static Core::Shaders::ReflectionInfo reflect(const std::vector<uint32_t>& spirv);
    static std::chrono::file_clock::time_point newestTimestamp(const std::vector<std::string>& deps);

    static uint64_t computeContentHash(const PreprocessedSource& src, const ShaderKey& key) {
        uint64_t h = Core::Hash::fnv1a(src.text);
        const uint8_t stageByte = static_cast<uint8_t>(key.stage);
        h = Core::Hash::combine64(h, stageByte);
        h = Core::Hash::combine64(h, Core::Hash::fnv1a(key.entry));
        h = Core::Hash::combine64(h, key.optionsHash);
        h = Core::Hash::combine64(h, Core::Hash::fnv1a(key.canonicalPath));
        return h; //
    }

    static uint64_t makeModuleKey(uint64_t blobHash, VkDevice device) {
        const uint64_t dev64 = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(device));
        struct Pair { uint64_t a, b; } p{blobHash, dev64};
        return Core::Hash::fnv1a(&p, sizeof(p));
    }
} // anonymous namespace

Core::Shaders::ShaderHandle Core::Shaders::ShaderLoader::get(const ShaderKey& key) {
    if (auto it = liveHandles_.find(key); it != liveHandles_.end())
        return it->second;

    // 1) Read root file + preprocess with glslang
    const std::string source = readWholeFile(key.canonicalPath);
    const auto defines = lookupDefines(key.optionsHash); // supply your real defines here
    PreprocessedSource src = GlslangPreprocess(source, key.canonicalPath, key.stage, key.entry, defines);

    // 2) Hash & fetch/compile blob
    const uint64_t contentHash = computeContentHash(src, key);

    std::shared_ptr<ShaderBlob> blob;
    if (auto bit = blobCache_.find(contentHash); bit != blobCache_.end()) {
        blob = bit->second;
    } else {
        std::vector<uint32_t> spirv = GlslangCompileToSpv(src.text, key.stage, key.entry);

        auto newBlob = std::make_shared<ShaderBlob>();
        newBlob->spirv           = std::move(spirv);
        newBlob->dependencies    = std::move(src.dependencies);
        newBlob->contentHash     = contentHash;
        newBlob->newestTimestamp = newestTimestamp(newBlob->dependencies);
        newBlob->reflect         = reflect(newBlob->spirv);

        blobCache_.emplace(contentHash, newBlob);
        blob = std::move(newBlob);
    }

    // 3) Build/find module (per-device)
    std::shared_ptr<ShaderModule> module;
    const uint64_t moduleKey = makeModuleKey(blob->contentHash, device_);

    if (auto mit = moduleCache_.find(moduleKey); mit != moduleCache_.end()) {
        module = mit->second;
    } else {
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = blob->spirv.size() * sizeof(uint32_t);
        ci.pCode    = blob->spirv.data();

        VkShaderModule vkModule = VK_NULL_HANDLE;
        if (vkCreateShaderModule(device_, &ci, nullptr, &vkModule) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateShaderModule failed");
        }

        auto newModule  = std::make_shared<ShaderModule>();
        newModule->vkModule = vkModule;
        newModule->device   = device_;
        newModule->blobHash = blob->contentHash;

        moduleCache_.emplace(moduleKey, newModule);
        module = std::move(newModule);
    }

    // 4) Handle
    ShaderHandle handle;
    handle.module  = std::move(module);
    handle.key     = key;
    handle.version = 1;

    auto [it, _] = liveHandles_.emplace(key, handle);
    return it->second;
}
