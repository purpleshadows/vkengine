#pragma once
class ShaderLoader;
namespace Core::Backend {
class Pipeline {
    public:
    private:
    ShaderLoader &shaderLoader;

    void createGraphicsPipeline();
};
}
